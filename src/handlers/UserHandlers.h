#ifndef USER_HANDLERS_H
#define USER_HANDLERS_H

#include "IHandler.h"
#include "../models/User.h"
#include "../auth/JWTManager.h"
#include "../database/DatabaseManager.h"

namespace handlers
{
/**
 * @class UserHandlers
 * @brief Handles user-related HTTP endpoints
 *
 * Implements user listing and search operations.
 * This class provides endpoints for retrieving user lists with pagination
 * and searching users by login.
 *
 * @note All methods are thread-safe and exception-safe unless otherwise specified.
 * @see IHandler
 */
class UserHandlers final : public IHandler
{
public:
    /**
     * @brief Constructs a UserHandlers instance with required dependencies
     * @param jwtManager Shared pointer to JWT token manager for authentication
     * @param dbManager Shared pointer to database manager for data persistence
     * @note Both dependencies must be non-null for proper operation
     * @throws std::invalid_argument if any parameter is null
     */
    UserHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept;

    /**
     * @brief Default virtual destructor
     * @note Ensures proper cleanup of inherited resources
     */
    virtual ~UserHandlers() noexcept override = default;

    /**
     * @brief Deleted copy constructor
     * @note UserHandlers should not be copied
     */
    UserHandlers(const UserHandlers&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note UserHandlers should not be copied
     */
    UserHandlers& operator=(const UserHandlers&) = delete;

    /**
     * @brief Default move constructor
     * @note UserHandlers can be moved
     */
    UserHandlers(UserHandlers&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note UserHandlers can be moved
     */
    UserHandlers& operator=(UserHandlers&&) noexcept = default;

    /**
     * @brief Main request handler for user endpoints
     * @param request HTTP request to process
     * @return boost::beast::http::response<boost::beast::http::string_body> HTTP response
     * @note Routes requests to appropriate handler methods based on endpoint and HTTP method
     * @warning This method never throws exceptions; errors are returned as HTTP error responses
     * @see handleGetUsers
     * @see handleSearchUsers
     */
    [[nodiscard]] virtual boost::beast::http::response<boost::beast::http::string_body> handleRequest(
        const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept override;

    /**
     * @brief Returns HTTP methods supported by user endpoints
     * @return std::vector<boost::beast::http::verb> List of supported HTTP methods
     * @note User endpoints support only GET method
     */
    [[nodiscard]] virtual std::vector<boost::beast::http::verb> getSupportedMethods() const noexcept override;

private:
    /**
     * @brief Handles user listing endpoint with pagination
     * @param request HTTP GET request with optional pagination parameters
     * @return HTTP response with user list and pagination metadata
     * @details Supported query parameters:
     * - page (int): Page number (default: 1)
     * - limit (int): Users per page (1-100, default: 50)
     * - search (string): Filter users by login substring (case-insensitive)
     * @note Requires Bearer token in Authorization header
     * @see getUsersPaginated
     * @see getTotalUsersCount
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleGetUsers(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles user search endpoint
     * @param request HTTP GET request with search query
     * @return HTTP response with search results
     * @details Supported query parameters:
     * - query (string): Search string (required)
     * - limit (int): Maximum results (1-50, default: 20)
     * @note Requires Bearer token in Authorization header
     * @note Returns users whose login contains the query string (case-insensitive)
     * @see searchUsers
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleSearchUsers(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Validates JWT access tokens for user operations
     * @param token JWT access token to validate
     * @param[out] userId User ID extracted from valid token
     * @return bool True if token is valid, false otherwise
     * @note Implements the pure virtual method from IHandler
     * @see auth::JWTManager::verifyAndDecode
     */
    [[nodiscard]] virtual bool isAuthTokenValid(const std::string& token, std::string& userId) const noexcept override;

    /**
     * @brief Retrieves paginated list of users with optional search filter
     * @param page Page number (1-based)
     * @param limit Number of users per page
     * @param search Optional search string to filter by login
     * @return std::vector<models::User> List of users for the requested page
     * @throws std::exception on database errors
     * @note Users are returned in descending order of creation (newest first)
     */
    [[nodiscard]] std::vector<models::User> getUsersPaginated(int page, int limit, const std::string& search = "") const;

    /**
     * @brief Searches users by login substring
     * @param query Search string to match against user logins
     * @param limit Maximum number of results to return
     * @return std::vector<models::User> List of matching users
     * @throws std::exception on database errors
     * @note Search is case-insensitive and uses SQL ILIKE operator
     * @note Results are ordered by login in ascending alphabetical order
     */
    [[nodiscard]] std::vector<models::User> searchUsers(const std::string& query, int limit) const;

    /**
     * @brief Retrieves total count of users with optional search filter
     * @param search Optional search string to filter by login
     * @return int Total number of users matching criteria
     * @note Logs errors to the system logger
     */
    [[nodiscard]] int getTotalUsersCount(const std::string& search = "") const noexcept;

private:
    std::shared_ptr<auth::JWTManager> jwtManager_;      ///< JWT token manager for authentication
    std::shared_ptr<database::DatabaseManager> dbManager_; ///< Database manager for user data storage
};
}

#endif // USER_HANDLERS_H