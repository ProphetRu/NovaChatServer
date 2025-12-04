#ifndef SESSION_H
#define SESSION_H

#include <memory>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include "Router.h"

namespace server
{
/**
 * @class Session
 * @brief Manages a single SSL/TLS connection session with a client
 *
 * Handles the complete lifecycle of an HTTP connection over SSL/TLS,
 * including handshake, request reading, response writing, and timeout management.
 * Implements asynchronous operations using Boost.Beast and Boost.Asio.
 *
 * @note Each Session instance is owned by a shared_ptr and manages its own lifetime
 * @warning Timeouts are enforced for handshake, read, write, and shutdown operations
 * @see Listener
 * @see Router
 */
class Session final : public std::enable_shared_from_this<Session>
{
public:
    /**
     * @brief Constructs a Session instance with a connected TCP socket
     * @param socket Connected TCP socket (moved into the session)
     * @param ssl_context SSL context for secure connection establishment
     * @param router Shared pointer to request router for HTTP handling
     */
    Session(boost::asio::ip::tcp::socket socket, boost::asio::ssl::context& ssl_context, std::shared_ptr<Router> router);

    /**
     * @brief Starts the session by initiating SSL handshake
     * @note Begins the asynchronous operation chain (handshake → read → write)
     */
    void start();

    /**
     * @brief Stops the session and cleans up resources
     * @note Safe to call multiple times
     */
    void stop() noexcept;

private:
    /**
     * @brief Callback handler for SSL handshake completion
     * @param ec Error code from handshake operation
     */
    void onHandshake(const boost::beast::error_code& ec);

    /**
     * @brief Initiates asynchronous HTTP request reading
     */
    void doRead();

    /**
     * @brief Callback handler for completed read operation
     * @param ec Error code from read operation
     * @param bytesTransferred Number of bytes read
     */
    void onRead(const boost::beast::error_code& ec, std::size_t bytesTransferred);

    /**
     * @brief Initiates asynchronous HTTP response writing
     */
    void doWrite();

    /**
     * @brief Callback handler for completed write operation
     * @param ec Error code from write operation
     * @param bytesTransferred Number of bytes written
     * @param isClose Flag indicating if connection should be closed after write
     */
    void onWrite(const boost::beast::error_code& ec, std::size_t bytesTransferred, bool isClose);

    /**
     * @brief Checks deadline timer and closes session on timeout
     * @note Recursively reschedules itself to maintain timeout checking
     */
    void checkDeadline();

    /**
     * @gracefully closes the SSL connection and TCP socket
     */
    void doClose();

    /**
     * @brief Logs incoming HTTP request details
     * @param request HTTP request to log
     */
    void logRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) const;

    /**
     * @brief Logs outgoing HTTP response details
     * @param response HTTP response to log
     */
    void logResponse(const boost::beast::http::response<boost::beast::http::string_body>& response) const;

    /**
     * @brief Retrieves client IP address for logging
     * @return std::string Client IP address or "unknown" on error
     */
    [[nodiscard]] std::string getClientIP() const;

private:
    boost::beast::ssl_stream<boost::beast::tcp_stream> stream_; ///< SSL/TLS stream over TCP
    std::shared_ptr<Router> router_;                            ///< HTTP request router
    boost::asio::steady_timer deadline_;                        ///< Timer for connection timeouts

    boost::beast::flat_buffer buffer_;  ///< Buffer for incoming request data
    boost::beast::http::request<boost::beast::http::string_body> request_; ///< Current HTTP request
    std::shared_ptr<boost::beast::http::response<boost::beast::http::string_body>> response_; ///< Current HTTP response

    bool isRunning_{ false }; ///< Session running state flag
};
}

#endif // SESSION_H