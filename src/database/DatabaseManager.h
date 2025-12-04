#ifndef DATABASE_MANAGER_H
#define DATABASE_MANAGER_H

#include <memory>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <pqxx/pqxx>

namespace database
{
/**
 * @class DatabaseManager
 * @brief Manages a thread-safe connection pool for PostgreSQL database
 *
 * This class provides a thread-safe connection pool for PostgreSQL database
 * operations using libpqxx. It manages a pool of database connections,
 * handles connection acquisition and release, and provides query execution
 * with proper error handling.
 *
 * @note The connection pool is initialized with a fixed number of connections
 *       and manages them using RAII principles.
 */
class DatabaseManager final
{
public:
    /**
     * @brief Constructs a DatabaseManager with connection pool
     * @param address Database server address or hostname
     * @param port Database server port
     * @param username Database username for authentication
     * @param password Database password for authentication
     * @param dbName Name of the database to connect to
     * @param maxConnections Maximum size of the connection pool
     * @param connectionTimeout Connection timeout in seconds
     * @throw std::runtime_error If connection pool initialization fails
     */
    DatabaseManager(const std::string& address,
        uint16_t port,
        const std::string& username,
        const std::string& password,
        const std::string& dbName,
        unsigned int maxConnections,
        unsigned int connectionTimeout);

    /**
     * @brief Destructor that ensures proper cleanup of connection pool
     */
    ~DatabaseManager() noexcept;

    /**
     * @brief Deleted copy constructor
     * @note DatabaseManager should not be copied
     */
    DatabaseManager(const DatabaseManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note DatabaseManager should not be copied
     */
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    /**
     * @brief Deleted move constructor
     * @note DatabaseManager should not be moved
     */
    DatabaseManager(DatabaseManager&&) noexcept = delete;

    /**
     * @brief Deleted move assignment operator
     * @note DatabaseManager should not be moved
     */
    DatabaseManager& operator=(DatabaseManager&&) noexcept = delete;

    /**
     * @brief Executes a SQL query using a connection from the pool
     * @param query SQL query string to execute
     * @return pqxx::result Result set from the query execution
     * @throw std::runtime_error If query execution fails or connection timeout occurs
     */
    pqxx::result executeQuery(const std::string& query);

    /**
     * @brief Performs a health check on the database
     * @return bool True if database is responsive, false otherwise
     * @note This method never throws exceptions
     */
    bool healthCheck() noexcept;

private:
    /**
     * @class ConnectionWrapper
     * @brief RAII wrapper for database connections
     *
     * This inner class provides automatic connection release back to the pool
     * when the wrapper goes out of scope. It ensures that connections are
     * always returned to the pool even in case of exceptions.
     */
    class ConnectionWrapper final
    {
    public:
        /**
         * @brief Constructs a ConnectionWrapper
         * @param conn Database connection to wrap
         * @param manager Pointer to the parent DatabaseManager
         */
        ConnectionWrapper(std::unique_ptr<pqxx::connection> conn, DatabaseManager* manager) noexcept
            : connection_{ std::move(conn) }, manager_{ manager }
        {
        }

        /**
         * @brief Destructor that automatically returns connection to pool
         */
        ~ConnectionWrapper() noexcept
        {
            if (connection_ && manager_)
            {
                manager_->releaseConnection(std::move(connection_));
            }
        }

        /**
         * @brief Arrow operator for accessing wrapped connection methods
         * @return pqxx::connection* Pointer to the wrapped connection
         */
        pqxx::connection* operator->() const noexcept
        {
            return connection_.get();
        }

        /**
         * @brief Gets the raw pointer to the wrapped connection
         * @return pqxx::connection* Pointer to the wrapped connection
         */
        pqxx::connection* get() const noexcept
        {
            return connection_.get();
        }

        /**
         * @brief Deleted copy constructor
         */
        ConnectionWrapper(const ConnectionWrapper&) = delete;

        /**
         * @brief Deleted copy assignment operator
         */
        ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;

        /**
         * @brief Deleted move constructor
         */
        ConnectionWrapper(ConnectionWrapper&&) = delete;

        /**
         * @brief Deleted move assignment operator
         */
        ConnectionWrapper& operator=(ConnectionWrapper&&) = delete;

    private:
        std::unique_ptr<pqxx::connection> connection_; ///< Wrapped database connection
        DatabaseManager* manager_; ///< Parent database manager for returning connection
    };

    /**
     * @brief Acquires a connection from the pool
     * @return std::unique_ptr<pqxx::connection> Acquired database connection
     * @throw std::runtime_error If connection acquisition times out
     */
    std::unique_ptr<pqxx::connection> acquireConnection();

    /**
     * @brief Releases a connection back to the pool
     * @param connection Database connection to release
     * @note If connection is invalid, creates a new one to maintain pool size
     */
    void releaseConnection(std::unique_ptr<pqxx::connection> connection);

    /**
     * @brief Handles connection errors and attempts recovery
     * @param connection Database connection that encountered an error
     */
    void handleConnectionError(std::unique_ptr<pqxx::connection> connection);

private:
    const std::string connectionString_; ///< PostgreSQL connection string
    const unsigned int maxConnections_; ///< Maximum connections in the pool
    const unsigned int connectionTimeout_; ///< Connection timeout in seconds

    std::queue<std::unique_ptr<pqxx::connection>> connectionPool_; ///< Queue of available connections
    std::mutex poolMutex_; ///< Mutex for thread-safe pool operations
    std::condition_variable poolCondition_; ///< Condition variable for connection waiting

    std::atomic<unsigned int> borrowedConnections_{}; ///< Counter for borrowed connections
};
}

#endif // DATABASE_MANAGER_H