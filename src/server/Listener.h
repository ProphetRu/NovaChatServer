#ifndef LISTENER_H
#define LISTENER_H

#include <memory>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include "Router.h"

namespace server
{
/**
 * @class Listener
 * @brief TCP listener that accepts incoming connections and creates SSL sessions
 *
 * Manages the server's TCP acceptor socket, listens for incoming connections,
 * and creates Session instances for each accepted connection. Implements
 * asynchronous I/O using Boost.Asio and supports SSL/TLS encryption.
 *
 * @note This class is thread-safe and uses strand-based synchronization
 * @warning Must be managed as a shared_ptr due to enable_shared_from_this
 * @see Session
 * @see Router
 */
class Listener final : public std::enable_shared_from_this<Listener>
{
public:
    /**
     * @brief Constructs a Listener instance with required dependencies
     * @param ioc Shared pointer to the I/O context for asynchronous operations
     * @param sslContext Shared pointer to SSL context for secure connections
     * @param endpoint Unique pointer to TCP endpoint configuration (address and port)
     * @param router Shared pointer to request router for handling HTTP requests
     * @throws std::invalid_argument if any parameter is null
     */
    Listener(std::shared_ptr<boost::asio::io_context> ioc,
        std::shared_ptr<boost::asio::ssl::context> sslContext,
        std::unique_ptr<boost::asio::ip::tcp::endpoint> endpoint,
        std::shared_ptr<Router> router);

    /**
     * @brief Destructor that ensures proper cleanup
     * @note Automatically stops the listener if still running
     */
    ~Listener() noexcept;

    /**
     * @brief Starts the listener to accept incoming connections
     * @throws std::runtime_error if listener cannot be started (bind/listen failures)
     * @note Begins asynchronous accept operations on the specified endpoint
     */
    void start();

    /**
     * @brief Stops the listener and closes all connections
     * @note Safe to call multiple times
     */
    void stop() noexcept;

private:
    /**
     * @brief Initiates an asynchronous accept operation
     * @note Only called when the listener is in running state
     */
    void doAccept();

    /**
     * @brief Callback handler for accepted connections
     * @param ec Error code from the accept operation
     * @param socket Accepted TCP socket ready for SSL handshake
     * @note Creates a new Session for each successful connection
     */
    void onAccept(const boost::beast::error_code& ec, boost::asio::ip::tcp::socket socket);

private:
    std::shared_ptr<boost::asio::io_context> ioc_;           ///< I/O context for asynchronous operations
    std::shared_ptr<boost::asio::ssl::context> sslContext_;  ///< SSL context for secure connections
    std::unique_ptr<boost::asio::ip::tcp::endpoint> endpoint_; ///< Network endpoint configuration

    std::shared_ptr<Router> router_;          ///< HTTP request router
    boost::asio::ip::tcp::acceptor acceptor_; ///< TCP acceptor socket
    bool isRunning_{ false };                 ///< Listener running state flag
};
}

#endif // LISTENER_H