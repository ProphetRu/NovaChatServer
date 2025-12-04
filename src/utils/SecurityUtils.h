#ifndef SECURITY_UTILS_H
#define SECURITY_UTILS_H

#include <string>

namespace utils
{
/**
 * @brief Utility class providing security-related operations for input sanitization.
 *
 * This class contains static methods for securing user inputs against common vulnerabilities.
 * It is designed to be used as a utility with no instantiation allowed (all methods are static).
 *
 * @final This class cannot be inherited from.
 */
class SecurityUtils final
{
public:
    /**
     * @brief Deleted constructor to enforce static usage.
     *
     * This class cannot be instantiated as it only provides static utility methods.
     */
    SecurityUtils() noexcept = delete;

    /**
     * @brief Sanitizes user input to prevent security vulnerabilities.
     *
     * This function performs multiple security validations on user input:
     * 1. Basic string sanitization (removing harmful characters)
     * 2. SQL injection detection
     * 3. Cross-site scripting (XSS) detection
     *
     * @note The function is noexcept and will not throw exceptions.
     * @note This function is marked [[nodiscard]] to ensure the return value is used.
     *
     * @param[in] input The raw user input string to sanitize.
     * @return std::string Sanitized string if input passes all security checks,
     *         empty string if security threats are detected,
     *         or original empty input if input was already empty.
     *
     * @warning This function provides basic sanitization. For critical applications,
     *          additional context-specific validation is recommended.
     *
     * @example
     * @code
     * auto safe_input = SecurityUtils::sanitizeUserInput(user_input);
     * if (safe_input.empty()) {
     *     // Handle potentially malicious input
     * }
     * @endcode
     */
    static [[nodiscard]] std::string sanitizeUserInput(const std::string& input) noexcept;
};
}

#endif // SECURITY_UTILS_H