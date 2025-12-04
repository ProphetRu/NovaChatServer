#include "Listener.h"
#include "Session.h"
#include "../utils/Logger.h"

namespace server
{
Listener::Listener(std::shared_ptr<boost::asio::io_context> ioc, std::shared_ptr<boost::asio::ssl::context> sslContext, std::unique_ptr<boost::asio::ip::tcp::endpoint> endpoint, std::shared_ptr<Router> router) :
    ioc_{ std::move(ioc) },
    sslContext_{ std::move(sslContext) },
	endpoint_{ std::move(endpoint) },
    router_{ std::move(router) },
    acceptor_{ boost::asio::make_strand(*ioc_) }
{
}

Listener::~Listener() noexcept
{
    stop();
}

void Listener::start()
{
    if (isRunning_) 
	{
        LOG_WARNING("Listener is already running");
        return;
    }

    boost::beast::error_code ec{};
    
    acceptor_.open(endpoint_->protocol(), ec);
    if (ec)
    {
        LOG_ERROR("Failed to open acceptor: " + ec.message());
        throw std::runtime_error{ ec.message() };
    }

    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec)
    {
        LOG_ERROR("Failed to set reuse address: " + ec.message());
        throw std::runtime_error{ ec.message() };
    }

    acceptor_.bind(*endpoint_, ec);
    if (ec)
    {
        LOG_ERROR("Failed to bind to endpoint: " + ec.message());
        throw std::runtime_error{ ec.message() };
    }

    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec)
    {
        LOG_ERROR("Failed to listen: " + ec.message());
        throw std::runtime_error{ ec.message() };
    }

    LOG_INFO("Listener created on " + endpoint_->address().to_string() + ":" + std::to_string(endpoint_->port()));

    isRunning_ = true;
    LOG_INFO("Starting listener...");

    doAccept();
}

void Listener::stop() noexcept
{
    if (!isRunning_)
    {
        return;
    }

    isRunning_ = false;

    boost::beast::error_code ec{};
    acceptor_.close(ec);
    if (ec) 
    {
        LOG_ERROR("Error stopping listener: " + ec.message());
    }
    else 
    {
        LOG_INFO("Listener stopped");
    }
}

void Listener::doAccept()
{
    if (isRunning_)
    {
        acceptor_.async_accept(
            boost::asio::make_strand(*ioc_),
            boost::beast::bind_front_handler(
                &Listener::onAccept,
                shared_from_this()));
    }
}

void Listener::onAccept(const boost::beast::error_code& ec, boost::asio::ip::tcp::socket socket)
{
    if (ec) 
    {
        if (ec != boost::asio::error::operation_aborted) 
        {
            LOG_ERROR("Accept error: " + ec.message());
        }

        return;
    }

    try 
    {
        LOG_DEBUG("New connection accepted from: " + socket.remote_endpoint().address().to_string());

        auto session{ std::make_shared<Session>(std::move(socket), *sslContext_, router_) };
        session->start();
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to create session: " + std::string{ e.what() });
    }

    // accepting the next connection
    if (isRunning_) 
    {
        doAccept();
    }
}
}
