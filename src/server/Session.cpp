#include "Session.h"
#include <nlohmann/json.hpp>
#include "../utils/Logger.h"

// additional option /bigobj
// in project properties -> C/C++ -> Command Line -> Additional Options
// to fix "fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
//
// additional option _WIN32_WINNT=0x0A00
// in project properties -> C/C++ -> Preprocessor -> Preprocessor Definitions
// to target Windows 10 or later for Boost.Asio

namespace server
{
constexpr std::chrono::seconds TIMEOUT_READ_WRITE{ 30 };
constexpr std::chrono::seconds TIMEOUT_HANDSHAKE{ 30 };
constexpr std::chrono::seconds TIMEOUT_SHUTDOWN{ 5 };

constexpr size_t MAX_BUFFER_SIZE{ 8192 };

Session::Session(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& ssl_context, std::shared_ptr<Router> router) :
    stream_{ std::move(socket), ssl_context },
    router_{ std::move(router) },
    deadline_{ stream_.get_executor() }
{
    buffer_.reserve(MAX_BUFFER_SIZE);
    deadline_.expires_at(std::chrono::steady_clock::time_point::max());
}

void Session::start()
{
    if (isRunning_)
    {
        LOG_WARNING("Session is already running");
        return;
    }

    isRunning_ = true;

    checkDeadline();

    // setting a timeout on a handshake
    deadline_.expires_after(std::chrono::seconds(TIMEOUT_HANDSHAKE));

    stream_.async_handshake(
        boost::asio::ssl::stream_base::server,
        boost::beast::bind_front_handler(
            &Session::onHandshake,
            shared_from_this()));
}

void Session::stop() noexcept
{
    if (!isRunning_)
    {
        return;
    }

    isRunning_ = false;

    stream_.next_layer().cancel();
    deadline_.cancel();

    LOG_INFO("Session stopped");
}

void Session::onHandshake(const boost::beast::error_code& ec)
{
    if (ec) 
    {
        if (ec != boost::asio::error::operation_aborted) 
        {
            LOG_ERROR("SSL handshake failed: " + ec.message());
        }

        return;
    }

    LOG_DEBUG("SSL handshake completed for client: " + getClientIP());
    doRead();
}

void Session::doRead()
{
    // resetting the request and buffer
    request_ = {};
    buffer_.consume(buffer_.size());

    // setting a timeout on reading
    deadline_.expires_after(std::chrono::seconds(TIMEOUT_READ_WRITE));

    boost::beast::http::async_read(
        stream_,
        buffer_,
        request_,
        boost::beast::bind_front_handler(
            &Session::onRead,
            shared_from_this()));
}

void Session::onRead(const boost::beast::error_code& ec, std::size_t bytesTransferred)
{
    if (ec) 
    {
        if (ec != boost::beast::http::error::end_of_stream && ec != boost::asio::error::operation_aborted) 
        {
            LOG_ERROR("Read error: " + ec.message());
        }

        doClose();
        return;
    }

    logRequest(request_);

    try 
    {
	    if (const auto handler{ router_->findHandler(request_) })
	    {
            response_ = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(handler->handleRequest(request_));
        }
        else
        {
            response_ = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(router_->handleNotFound(request_));
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error handling request: " + std::string{ e.what() });

        nlohmann::json error_json{};
        error_json["status"] = "error";
        error_json["code"] = "INTERNAL_ERROR";
        error_json["message"] = "Internal server error";

        response_ = std::make_shared<boost::beast::http::response<boost::beast::http::string_body>>(boost::beast::http::status::internal_server_error, request_.version());
        response_->set(boost::beast::http::field::content_type, "application/json");
        response_->set(boost::beast::http::field::access_control_allow_origin, "*");
        response_->body() = error_json.dump();
        response_->prepare_payload();
    }

    logResponse(*response_);

    doWrite();
}

void Session::doWrite()
{
    deadline_.expires_after(std::chrono::seconds(TIMEOUT_READ_WRITE));

    auto self{ shared_from_this() };

    boost::beast::http::async_write(
        stream_,
        *response_,
        [self](const boost::beast::error_code& ec, std::size_t bytesTransferred) 
    {
        self->onWrite(ec, bytesTransferred, self->response_->need_eof());
    });
}

void Session::onWrite(const boost::beast::error_code& ec, std::size_t bytesTransferred, bool isClose)
{
    if (ec) 
    {
        LOG_ERROR("Write error: " + ec.message());
        return;
    }

    if (isClose) 
    {
        doClose();
        return;
    }

    // reading the next request (keep-alive)
    doRead();
}

void Session::checkDeadline()
{
    if (!isRunning_)
    {
        return;
    }

    if (deadline_.expiry() <= std::chrono::steady_clock::now()) 
    {
        LOG_DEBUG("Session timeout for client: " + getClientIP());
        doClose();
        return;
    }

    deadline_.async_wait(
        [self = shared_from_this()](const boost::beast::error_code& ec) 
    {
        if (!ec) 
        {
            self->checkDeadline();
        }
    });
}

void Session::doClose()
{
    if (!isRunning_)
    {
        return;
    }

    isRunning_ = false;

    deadline_.expires_after(std::chrono::seconds(TIMEOUT_SHUTDOWN));

    auto self{ shared_from_this() };

    stream_.async_shutdown(
        [self](const boost::beast::error_code& ec) 
    {
        if (ec && ec != boost::asio::error::eof && ec != boost::asio::ssl::error::stream_truncated) 
        {
            LOG_ERROR("SSL shutdown error: " + ec.message());
        }

        self->stream_.next_layer().close();
        self->deadline_.cancel();

        LOG_DEBUG("Session closed for client: " + self->getClientIP());
    });
}

void Session::logRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) const
{
    const auto clientIP{ getClientIP() };
    const auto method{ request.method_string() };
    const auto target{ request.target() };

    std::stringstream logEntry{};
    logEntry << clientIP << " - - ["
        << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count()
        << "] \"" << method << " " << target << " HTTP/"
        << (request.version() / 10) << "." << (request.version() % 10) << "\"";

    LOG_ACCESS(logEntry.str());

    LOG_DEBUG(std::format("Request from {}: {} {}", clientIP, method, target));
}

void Session::logResponse(const boost::beast::http::response<boost::beast::http::string_body>& response) const
{
    const auto clientIP{ getClientIP() };
    const auto statusCode{ response.result_int() };

    std::stringstream log_entry{};
    log_entry << " " << statusCode << " " << response.body();

    LOG_ACCESS(log_entry.str());

    LOG_DEBUG("Response to " + clientIP + ": " + response.body());
}

std::string Session::getClientIP() const
{
    try 
    {
        boost::beast::error_code ec{};
        const auto endpoint{ stream_.next_layer().socket().remote_endpoint(ec) };
        if (!ec) 
        {
            return endpoint.address().to_string();
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting client IP: " + std::string{ e.what() });
    }

    return "unknown";
}
}
