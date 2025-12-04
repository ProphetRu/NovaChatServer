#ifndef VALIDATORS_H
#define VALIDATORS_H

#include <string>
#include <array>

namespace utils
{
/**
 * @class Validators
 * @brief Provides validation and sanitization utilities for input data
 *
 * Contains static methods for validating user inputs (logins, passwords, UUIDs, etc.)
 * and detecting security threats like SQL injection and XSS attacks.
 * All methods are thread-safe and noexcept.
 *
 * @note This is a utility class with only static methods
 */
class Validators final
{
public:
    /**
     * @brief Deleted default constructor
     * @note This is a utility class with only static methods
     */
    Validators() noexcept = delete;

    /**
     * @brief Validates user login format
     * @param login Login string to validate
     * @return bool True if login is valid, false otherwise
     * @details Validates that login is 3-50 characters and contains only letters, numbers, and underscores
     */
    static [[nodiscard]] bool isLoginValid(const std::string& login) noexcept;

    /**
     * @brief Validates password strength and format
     * @param password Password string to validate
     * @return bool True if password is valid, false otherwise
     * @details Validates that password is 6-128 characters and contains at least one letter and one digit
     */
    static [[nodiscard]] bool isPasswordValid(const std::string& password) noexcept;

    /**
     * @brief Validates UUID format
     * @param uuid UUID string to validate
     * @return bool True if UUID is valid, false otherwise
     * @details Validates standard UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
     */
    static [[nodiscard]] bool isUUIDValid(const std::string& uuid) noexcept;

    /**
     * @brief Validates message length
     * @param message Message string to validate
     * @param maxLength Maximum allowed message length (default: 4096)
     * @return bool True if message length is valid, false otherwise
     */
    static [[nodiscard]] bool isMessageLengthValid(const std::string& message, size_t maxLength = 4096) noexcept;

    /**
     * @brief Sanitizes string by removing dangerous characters and escaping special ones
     * @param input String to sanitize
     * @return std::string Sanitized string safe for database storage
     */
    static [[nodiscard]] std::string sanitizeString(const std::string& input) noexcept;

    /**
     * @brief Detects potential SQL injection attempts in input string
     * @param input String to check for SQL injection patterns
     * @return bool True if SQL injection is detected, false otherwise
     * @note Checks for common SQL keywords in context-aware manner
     */
    static [[nodiscard]] bool isSQLInjection(const std::string& input) noexcept;

    /**
     * @brief Detects potential XSS (Cross-Site Scripting) attacks in input string
     * @param input String to check for XSS patterns
     * @return bool True if XSS is detected, false otherwise
     */
    static [[nodiscard]] bool isXSS(const std::string& input) noexcept;

private:
    static constexpr std::array<std::string_view, 16> sqlKeywords_ ///< Common SQL keywords for injection detection
    {
        "SELECT", "INSERT", "UPDATE", "DELETE", "DROP", "UNION", "OR", "AND",
        "WHERE", "FROM", "TABLE", "DATABASE", "ALTER", "CREATE", "EXEC", "SCRIPT"
    };

    static constexpr std::array<std::string_view, 9> xssPatterns_ ///< Common XSS attack patterns for detection
    {
        "<script", "javascript:", "onload=", "onerror=", "onclick=",
        "eval(", "alert(", "document.cookie", "<iframe"
    };
};
}

#endif // VALIDATORS_H