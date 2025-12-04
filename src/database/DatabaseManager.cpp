#include "DatabaseManager.h"
#include "../utils/Logger.h"
#include <format>
#include <chrono>
#include <stdexcept>

namespace database
{
DatabaseManager::DatabaseManager(const std::string& address, uint16_t port, const std::string& username, const std::string& password, const std::string& dbName, unsigned int maxConnections, unsigned int connectionTimeout) :
	connectionString_{ std::format("postgresql://{}:{}@{}:{}/{}?connect_timeout={}&sslmode=require", username, password, address, port, dbName, connectionTimeout) },
    maxConnections_{ maxConnections },
    connectionTimeout_{ connectionTimeout }
{
    LOG_DEBUG("Connection string: " + connectionString_);

    LOG_INFO("Initializing database connection pool with " + std::to_string(maxConnections_) + " connections");

    try 
    {
        for (auto _ : std::ranges::views::iota(0u, maxConnections_))
        {
            auto conn{ std::make_unique<pqxx::connection>(connectionString_) };
            if (!conn->is_open()) 
            {
                throw std::runtime_error("Failed to establish database connection");
            }

            conn->set_client_encoding("UTF8");

            std::lock_guard lock{ poolMutex_ };
            connectionPool_.push(std::move(conn));
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to create initial connection: " + std::string{ e.what() });
        throw std::runtime_error(std::format("Database connection failed: {}", e.what()));
    }

    if (connectionPool_.empty())
    {
        LOG_ERROR("No database connections could be established during initialization");
        throw std::runtime_error{ "No database connections could be established during initialization" };
    }

    LOG_INFO("Database connection pool initialized successfully");
}

DatabaseManager::~DatabaseManager() noexcept
{
    poolCondition_.notify_all();

    std::lock_guard lock{ poolMutex_ };
    while (!connectionPool_.empty()) 
    {
        connectionPool_.pop();
    }

    LOG_INFO("Database manager shutdown completed");
}

pqxx::result DatabaseManager::executeQuery(const std::string& query)
{
    auto connection{ acquireConnection() };

    try 
    {
        pqxx::work transaction{ *connection };
        auto result{ transaction.exec(query) };
        transaction.commit();

        releaseConnection(std::move(connection));

        LOG_DEBUG("Query executed successfully: " + query);
        return result;
    }
    catch (const pqxx::sql_error& e)
    {
        handleConnectionError(std::move(connection));
        LOG_ERROR("SQL error in query '" + query + "': " + e.what());
        throw std::runtime_error(std::format("Query execution failed: {}", e.what()));
    }
    catch (const std::exception& e) 
    {
        handleConnectionError(std::move(connection));
        LOG_ERROR("Unexpected error in query '" + query + "': " + e.what());
        throw std::runtime_error(std::format("Query execution failed: {}", e.what()));
    }
}

bool DatabaseManager::healthCheck() noexcept
{
    try
    {
	    const auto result{ executeQuery("SELECT 1") };
        return !result.empty() && result[0][0].as<int>() == 1;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Database health check failed: " + std::string{ e.what() });
        return false;
    }
}

std::unique_ptr<pqxx::connection> DatabaseManager::acquireConnection()
{
    std::unique_lock lock{ poolMutex_ };

    const auto timeout{ std::chrono::seconds(connectionTimeout_) };
    if (!poolCondition_.wait_for(lock, timeout, [this]()
    {
        return !connectionPool_.empty();
    }))
    {
        throw std::runtime_error{ "Timeout waiting for database connection" };
    }

    if (connectionPool_.empty()) 
    {
        // create new connection if pool is empty but we haven't reached max connections
        auto conn{ std::make_unique<pqxx::connection>(connectionString_) };
        if (!conn->is_open()) 
        {
            throw std::runtime_error("Failed to create new database connection");
        }

        conn->set_client_encoding("UTF8");

        ++borrowedConnections_;
        LOG_DEBUG("Database connection borrowed. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));

        return conn;
    }

    auto conn{ std::move(connectionPool_.front()) };
    connectionPool_.pop();

    ++borrowedConnections_;
    LOG_DEBUG("Database connection borrowed. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));

    return conn;
}

void DatabaseManager::releaseConnection(std::unique_ptr<pqxx::connection> connection)
{
    {
        std::lock_guard lock{ poolMutex_ };
        if (connection && connection->is_open()) 
        {
            connectionPool_.emplace(std::move(connection));

            --borrowedConnections_;
            LOG_DEBUG("Database connection returned to pool. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));
        }
        else 
        {
            // Connection is bad, create a new one
            try 
            {
	            if (auto newConn{ std::make_unique<pqxx::connection>(connectionString_) }; newConn->is_open()) 
                {
                    newConn->set_client_encoding("UTF8");
	            	connectionPool_.emplace(std::move(newConn));

                    --borrowedConnections_;
                    LOG_DEBUG("Database connection returned to pool. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));
                }
            }
            catch (...) 
            {
                LOG_WARNING("Invalid connection discarded from pool");
            }
        }
    }

    poolCondition_.notify_one();
}

void DatabaseManager::handleConnectionError(std::unique_ptr<pqxx::connection> connection)
{
    {
        std::lock_guard lock{ poolMutex_ };
        // try to recreate the connection if it's in a bad state
        try 
        {
            if (!connection || !connection->is_open()) 
            {
	            if (auto newConn{ std::make_unique<pqxx::connection>(connectionString_) }; newConn->is_open()) 
                {
                    newConn->set_client_encoding("UTF8");
	            	connectionPool_.emplace(std::move(newConn));

                    --borrowedConnections_;
                    LOG_DEBUG("Database connection returned to pool. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));
                }
            }
            else 
            {
                connection->set_client_encoding("UTF8");
            	connectionPool_.emplace(std::move(connection));

                --borrowedConnections_;
                LOG_DEBUG("Database connection returned to pool. Borrowed: " + std::to_string(borrowedConnections_) + ", Available: " + std::to_string(connectionPool_.size()));
            }
        }
        catch (...) 
        {
            LOG_WARNING("Invalid connection discarded from pool");
        }
    }

    poolCondition_.notify_one();
}
}