#ifndef JWT_MANAGER_H
#define JWT_MANAGER_H

#include <string>
#include <chrono>
#include <unordered_map>
#include <mutex>

namespace auth
{
/**
 * @class JWTManager
 * @brief Manager for JSON Web Tokens (JWT) operations
 *
 * Provides functionality for generating, verifying, and managing JWT tokens
 * for authentication and authorization. Supports both access and refresh tokens
 * with configurable expiration times.
 *
 * @note For production environments, consider using Redis or a database
 *       for token blacklisting instead of in-memory storage.
 */
class JWTManager final
{
public:
    /**
     * @brief Constructs a new JWTManager instance
     * @param secretKey The secret key used for signing tokens
     * @param accessTokenExpiryMinutes Access token expiration time in minutes
     * @param refreshTokenExpiryDays Refresh token expiration time in days
     * @throw std::invalid_argument If secretKey is empty or too short
     */
    JWTManager(const std::string& secretKey, unsigned int accessTokenExpiryMinutes, unsigned int refreshTokenExpiryDays);

    /**
     * @brief Default destructor
     */
    ~JWTManager() noexcept = default;

    /**
     * @brief Deleted copy constructor
     */
    JWTManager(const JWTManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     */
    JWTManager& operator=(const JWTManager&) = delete;

    /**
     * @brief Deleted move constructor
     */
    JWTManager(JWTManager&&) noexcept = delete;

    /**
     * @brief Deleted move assignment operator
     */
    JWTManager& operator=(JWTManager&&) noexcept = delete;

    /**
     * @brief Generates an access token for a user
     * @param userId Unique identifier of the user
     * @param login User's login/username
     * @return std::string Signed JWT access token
     * @throw std::runtime_error If token generation fails
     */
    [[nodiscard]] std::string generateAccessToken(const std::string& userId, const std::string& login) const;

    /**
     * @brief Generates a refresh token for a user
     * @param userId Unique identifier of the user
     * @return std::string Signed JWT refresh token
     * @throw std::runtime_error If token generation fails
     */
    [[nodiscard]] std::string generateRefreshToken(const std::string& userId) const;

    /**
     * @struct TokenPayload
     * @brief Represents decoded JWT token payload
     *
     * Contains the claims extracted from a JWT token after verification
     */
    struct TokenPayload final
    {
        std::string userID; ///< Unique user identifier
        std::string login;  ///< User's login name
        std::string type;   ///< Token type: "access" or "refresh"
        std::chrono::system_clock::time_point expiresAt{}; ///< Token expiration timestamp
        bool isValid{};     ///< Indicates if token is valid

        /**
         * @brief Checks if the token is an access token
         * @return bool True if token type is "access"
         */
        [[nodiscard]] bool isAccessToken() const noexcept { return type == "access"; }

        /**
         * @brief Checks if the token is a refresh token
         * @return bool True if token type is "refresh"
         */
        [[nodiscard]] bool isRefreshToken() const noexcept { return type == "refresh"; }
    };

    /**
     * @brief Verifies and decodes a JWT token
     * @param token The JWT token string to verify
     * @return TokenPayload Decoded token payload with validation status
     * @throw std::runtime_error If token parsing or verification fails
     */
    [[nodiscard]] TokenPayload verifyAndDecode(const std::string& token);

    /**
     * @brief Extracts expiration time from a token
     * @param token The JWT token to examine
     * @return std::chrono::system_clock::time_point Token expiration timestamp
     * @throw std::runtime_error If token parsing fails
     */
    [[nodiscard]] std::chrono::system_clock::time_point getTokenExpiry(const std::string& token);

    /**
     * @brief Adds a token to the blacklist (for logout functionality)
     * @param token The token to blacklist
     * @note Thread-safe operation
     */
    void addTokenToBlacklist(const std::string& token) noexcept;

    /**
     * @brief Checks if a token is blacklisted
     * @param token The token to check
     * @return bool True if token is in the blacklist
     * @note Thread-safe operation
     */
    [[nodiscard]] bool isTokenBlacklisted(const std::string& token) noexcept;

    /**
     * @brief Removes expired tokens from the blacklist
     * @note Thread-safe operation
     */
    void cleanupExpiredBlacklistedTokens() noexcept;

private:
    /**
     * @brief Calculates access token expiration time
     * @return std::chrono::system_clock::time_point Future timestamp
     */
    [[nodiscard]] std::chrono::system_clock::time_point getAccessTokenExpiry() const noexcept;

    /**
     * @brief Calculates refresh token expiration time
     * @return std::chrono::system_clock::time_point Future timestamp
     */
    [[nodiscard]] std::chrono::system_clock::time_point getRefreshTokenExpiry() const noexcept;

private:
    const std::string secretKey_;                 ///< Secret key for token signing
    const unsigned int accessTokenExpiryMinutes_; ///< Access token lifetime in minutes
    const unsigned int refreshTokenExpiryDays_;   ///< Refresh token lifetime in days

    /**
     * @brief In-memory token blacklist storage
     * @note In production, use Redis or database for persistence
     */
    std::unordered_map<std::string, std::chrono::system_clock::time_point> blacklistedTokens_;

    /**
     * @brief Mutex for thread-safe access to blacklist
     */
    std::mutex blacklistMutex_;
};
}

#endif // JWT_MANAGER_H
