#ifndef SERVER_H
#define SERVER_H

#include <memory>
#include <vector>
#include <thread>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl.hpp>
#include "../config/ConfigManager.h"
#include "../database/DatabaseManager.h"
#include "../auth/JWTManager.h"
#include "Listener.h"
#include "Router.h"

namespace server
{
/**
 * @class Server
 * @brief Main server class that orchestrates all components of the chat application
 *
 * Manages server lifecycle including startup, graceful shutdown, and resource management.
 * Coordinates SSL configuration, request routing, connection listening, and worker threads.
 * Implements the main server loop and provides the top-level API for controlling the server.
 *
 * @note Thread-safe operations with atomic state management
 * @warning Ensure proper shutdown sequence to avoid resource leaks
 * @see Listener
 * @see Router
 * @see ConfigManager
 */
class Server final
{
public:
    /**
     * @brief Constructs a Server instance with required dependencies
     * @param config Unique pointer to configuration manager
     * @param dbManager Shared pointer to database manager
     * @param jwtManager Shared pointer to JWT token manager
     * @throws std::runtime_error if initialization fails
     */
    Server(std::unique_ptr<config::ConfigManager> config, std::shared_ptr<database::DatabaseManager> dbManager, std::shared_ptr<auth::JWTManager> jwtManager);

    /**
     * @brief Destructor that ensures proper cleanup
     * @note Automatically stops the server if still running
     */
    ~Server() noexcept;

    /**
     * @brief Deleted copy constructor
     * @note Server should not be copied
     */
    Server(const Server&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note Server should not be copied
     */
    Server& operator=(const Server&) = delete;

    /**
     * @brief Deleted move constructor
     * @note Server should not be moved
     */
    Server(Server&&) = delete;

    /**
     * @brief Deleted move assignment operator
     * @note Server should not be moved
     */
    Server& operator=(Server&&) = delete;

    /**
     * @brief Starts the server and begins accepting connections
     * @throws std::runtime_error if server cannot be started
     * @note Initializes SSL, router, listener, and starts worker threads
     */
    void start();

    /**
     * @brief Stops the server gracefully
     * @note Waits for active connections to complete before shutdown
     */
    void stop() noexcept;

    /**
     * @brief Checks if server is currently running
     * @return bool True if server is running, false otherwise
     */
    [[nodiscard]] bool isRunning() const noexcept;

private:
    /**
     * @brief Initializes SSL/TLS context with certificates and security settings
     * @throws std::runtime_error if SSL configuration fails
     */
    void initializeSSL() const;

    /**
     * @brief Initializes request router and registers all HTTP handlers
     * @throws std::runtime_error if router initialization fails
     * @see AuthHandlers
     * @see UserHandlers
     * @see MessageHandlers
     */
    void initializeRouter();

    /**
     * @brief Initializes TCP listener for incoming connections
     * @throws std::runtime_error if listener cannot be created
     */
    void initializeListener();

    /**
     * @brief Performs graceful shutdown sequence
     * @note Stops listener, waits for active connections, stops I/O context
     */
    void gracefulShutdown() noexcept;

    /**
     * @brief Waits for graceful shutdown within timeout limit
     * @return bool True if shutdown completed gracefully, false if timed out
     */
    [[nodiscard]] bool waitForGracefulShutdown() const noexcept;

private:
    std::unique_ptr<config::ConfigManager> config_;        ///< Configuration manager for server settings
    std::shared_ptr<database::DatabaseManager> dbManager_; ///< Database manager for data persistence
    std::shared_ptr<auth::JWTManager> jwtManager_;         ///< JWT manager for authentication tokens

    std::shared_ptr<boost::asio::io_context> ioc_;           ///< I/O context for asynchronous operations
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_; ///< Work guard to keep I/O context active

    std::shared_ptr<boost::asio::ssl::context> sslContext_; ///< SSL context for secure connections

    std::vector<std::jthread> threads_;                     ///< Worker threads for handling I/O operations

    std::shared_ptr<Listener> listener_;                    ///< TCP listener for incoming connections
    std::shared_ptr<Router> router_;                        ///< HTTP request router

    std::atomic<bool> isRunning_{ false };                  ///< Server running state flag
};
}

#endif // SERVER_H