#ifndef AUTH_HANDLERS_H
#define AUTH_HANDLERS_H

#include "IHandler.h"
#include "../auth/JWTManager.h"
#include "../database/DatabaseManager.h"

namespace handlers
{
/**
 * @class AuthHandlers
 * @brief Handles authentication-related HTTP endpoints
 *
 * Implements user authentication, registration, token management, and account operations.
 * This class provides endpoints for user registration, login, token refresh, logout,
 * password changes, and account deletion.
 *
 * @note All methods are thread-safe and exception-safe unless otherwise specified.
 * @see IHandler
 */
class AuthHandlers final : public IHandler
{
public:
    /**
     * @brief Constructs an AuthHandlers instance with required dependencies
     * @param jwtManager Shared pointer to JWT token manager
     * @param dbManager Shared pointer to database manager
     * @note Both dependencies must be non-null for proper operation
     * @throws std::invalid_argument if any parameter is null
     */
    AuthHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept;

    /**
     * @brief Default virtual destructor
     * @note Ensures proper cleanup of inherited resources
     */
    virtual ~AuthHandlers() noexcept override = default;

    /**
     * @brief Deleted copy constructor
     * @note AuthHandlers should not be copied
     */
    AuthHandlers(const AuthHandlers&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note AuthHandlers should not be copied
     */
    AuthHandlers& operator=(const AuthHandlers&) = delete;

    /**
     * @brief Default move constructor
     * @note AuthHandlers can be moved
     */
    AuthHandlers(AuthHandlers&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note AuthHandlers can be moved
     */
    AuthHandlers& operator=(AuthHandlers&&) noexcept = default;

    /**
     * @brief Main request handler for authentication endpoints
     * @param request HTTP request to process
     * @return boost::beast::http::response<boost::beast::http::string_body> HTTP response
     * @note Routes requests to appropriate handler methods based on endpoint and HTTP method
     * @warning This method never throws exceptions; errors are returned as HTTP error responses
     * @see handleRegister
     * @see handleLogin
     * @see handleRefresh
     * @see handleLogout
     * @see handleChangePassword
     * @see handleDeleteAccount
     */
    [[nodiscard]] virtual boost::beast::http::response<boost::beast::http::string_body> handleRequest(
        const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept override;

    /**
     * @brief Returns HTTP methods supported by authentication endpoints
     * @return std::vector<boost::beast::http::verb> List of supported HTTP methods
     * @note Authentication endpoints support POST, PUT, and DELETE methods
     */
    [[nodiscard]] virtual std::vector<boost::beast::http::verb> getSupportedMethods() const noexcept override;

private:
    // Endpoint handlers

    /**
     * @brief Handles user registration endpoint
     * @param request HTTP POST request with registration data
     * @return HTTP response indicating registration success or failure
     * @note Validates input, checks for existing users, and creates new user account
     * @details Expected JSON body: {"login": string, "password": string}
     * @see models::User::createFromCredentials
     * @see utils::Validators::isLoginValid
     * @see utils::Validators::isPasswordValid
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleRegister(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles user login endpoint
     * @param request HTTP POST request with login credentials
     * @return HTTP response with access and refresh tokens on successful authentication
     * @details Expected JSON body: {"login": string, "password": string}
     * @see utils::PasswordHasher::isPasswordValid
     * @see auth::JWTManager::generateAccessToken
     * @see auth::JWTManager::generateRefreshToken
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleLogin(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles token refresh endpoint
     * @param request HTTP POST request with refresh token
     * @return HTTP response with new access and refresh tokens
     * @details Expected JSON body: {"refresh_token": string}
     * @note Invalidates the old refresh token after successful refresh
     * @see auth::JWTManager::verifyAndDecode
     * @see auth::JWTManager::generateAccessToken
     * @see auth::JWTManager::generateRefreshToken
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleRefresh(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles user logout endpoint
     * @param request HTTP POST request with access and refresh tokens
     * @return HTTP response confirming logout
     * @note Invalidates both access and refresh tokens
     * @details Requires Bearer token in Authorization header and refresh_token in JSON body
     * @see auth::JWTManager::addTokenToBlacklist
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleLogout(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles password change endpoint
     * @param request HTTP PUT request with old and new passwords
     * @return HTTP response confirming password change
     * @details Expected JSON body: {"old_password": string, "new_password": string}
     * @note Requires Bearer token in Authorization header
     * @see utils::Validators::isPasswordValid
     * @see utils::PasswordHasher::hashPassword
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleChangePassword(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Handles account deletion endpoint
     * @param request HTTP DELETE request with authentication token
     * @return HTTP response confirming account deletion
     * @note Requires Bearer token in Authorization header
     * @warning This operation is irreversible and deletes all user data
     * @see auth::JWTManager::addTokenToBlacklist
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleDeleteAccount(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    // Authentication and validation methods

    /**
     * @brief Validates JWT access tokens
     * @param token JWT access token to validate
     * @param[out] userId User ID extracted from valid token
     * @return bool True if token is valid, false otherwise
     * @note Implements the pure virtual method from IHandler
     * @see auth::JWTManager::verifyAndDecode
     */
    [[nodiscard]] virtual bool isAuthTokenValid(const std::string& token, std::string& userId) const noexcept override;

    /**
     * @brief Checks if a user with given login exists
     * @param login User login to check
     * @return bool True if user exists, false otherwise
     * @note Logs errors to the system logger
     */
    [[nodiscard]] bool isUserExists(const std::string& login) const noexcept;

    /**
     * @brief Validates current password for a user
     * @param userId User ID
     * @param password Password to validate
     * @return bool True if password matches, false otherwise
     * @note Logs errors to the system logger
     * @see utils::PasswordHasher::isPasswordValid
     */
    [[nodiscard]] bool isCurrentPasswordValid(const std::string& userId, const std::string& password) const noexcept;

    /**
     * @brief Stores refresh token in database for later validation
     * @param userId User ID associated with the token
     * @param refreshToken Refresh token to store
     * @return bool True if storage succeeded, false otherwise
     * @note Tokens are hashed before storage for security
     * @see utils::PasswordHasher::sha256
     * @see auth::JWTManager::getTokenExpiry
     */
    [[nodiscard]] bool storeRefreshToken(const std::string& userId, const std::string& refreshToken) const noexcept;

    /**
     * @brief Invalidates/removes refresh token from database
     * @param refreshToken Refresh token to invalidate
     * @return bool True if invalidation succeeded, false otherwise
     * @note Logs errors to the system logger
     * @see utils::PasswordHasher::sha256
     */
    [[nodiscard]] bool invalidateRefreshToken(const std::string& refreshToken) const noexcept;

private:
    std::shared_ptr<auth::JWTManager> jwtManager_;      ///< JWT token manager for token operations
    std::shared_ptr<database::DatabaseManager> dbManager_; ///< Database manager for data persistence
};
}

#endif // AUTH_HANDLERS_H