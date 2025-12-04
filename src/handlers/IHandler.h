#ifndef IHANDLER_H
#define IHANDLER_H

#include <string>
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>

namespace handlers
{
/**
 * @class IHandler
 * @brief Abstract base class for HTTP request handlers
 *
 * This interface defines the contract for all HTTP request handlers in the application.
 * It provides common functionality for creating JSON responses, handling authentication,
 * and validating request data while allowing derived classes to implement specific
 * endpoint logic.
 *
 * @note All methods are thread-safe and exception-safe unless otherwise specified.
 */
class IHandler
{
public:
    /**
     * @brief Default constructor
     */
    IHandler() noexcept = default;

    /**
     * @brief Virtual destructor for proper polymorphic destruction
     */
    virtual ~IHandler() noexcept = default;

    /**
     * @brief Deleted copy constructor
     * @note Handlers should not be copied
     */
    IHandler(const IHandler&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note Handlers should not be copied
     */
    IHandler& operator=(const IHandler&) = delete;

    /**
     * @brief Default move constructor
     * @note Handlers can be moved
     */
    IHandler(IHandler&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note Handlers can be moved
     */
    IHandler& operator=(IHandler&&) noexcept = default;

    /**
     * @brief Pure virtual method for handling HTTP requests
     * @param request HTTP request to handle
     * @return boost::beast::http::response<boost::beast::http::string_body> HTTP response
     * @note This method must be implemented by derived classes and should never throw exceptions
     */
    [[nodiscard]] virtual boost::beast::http::response<boost::beast::http::string_body> handleRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept = 0;

    /**
     * @brief Gets the HTTP methods supported by this handler
     * @return std::vector<boost::beast::http::verb> List of supported HTTP methods
     */
    [[nodiscard]] virtual std::vector<boost::beast::http::verb> getSupportedMethods() const noexcept = 0;

protected:
    /**
     * @brief Validates an authentication token
     * @param token JWT token to validate
     * @param userId[out] User ID extracted from valid token
     * @return bool True if token is valid, false otherwise
     * @note Must be implemented by derived classes to provide authentication logic
     */
    [[nodiscard]] virtual bool isAuthTokenValid(const std::string& token, std::string& userId) const noexcept = 0;

    /**
     * @brief Creates a success HTTP response with JSON body
     * @param data JSON data to include in response
     * @param status HTTP status code (default: 200 OK)
     * @param message Optional success message
     * @return boost::beast::http::response<boost::beast::http::string_body> Formatted HTTP response
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> createSuccessResponse(
        const nlohmann::json& data,
        boost::beast::http::status status = boost::beast::http::status::ok,
        const std::string& message = "") const noexcept;

    /**
     * @brief Creates an error HTTP response with JSON body
     * @param status HTTP error status code
     * @param errorCode Application-specific error code
     * @param message Human-readable error message
     * @return boost::beast::http::response<boost::beast::http::string_body> Formatted HTTP error response
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> createErrorResponse(
        boost::beast::http::status status,
        const std::string& errorCode,
        const std::string& message) const noexcept;

    /**
     * @brief Creates a generic JSON HTTP response
     * @param json JSON object for response body
     * @param status HTTP status code (default: 200 OK)
     * @return boost::beast::http::response<boost::beast::http::string_body> Formatted HTTP response
     * @note Includes CORS headers and proper content type
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> createJsonResponse(
        const nlohmann::json& json,
        boost::beast::http::status status = boost::beast::http::status::ok) const noexcept;

    /**
     * @brief Validates if a request body contains valid JSON
     * @param body Request body string
     * @param json[out] Parsed JSON object if valid
     * @return bool True if body contains valid JSON, false otherwise
     */
    [[nodiscard]] bool isJsonBodyValid(const std::string& body, nlohmann::json& json) const noexcept;

    /**
     * @brief Extracts Bearer token from Authorization header
     * @param request HTTP request
     * @return std::string Token string or empty string if not found/invalid
     * @note Expects "Authorization: Bearer <token>" header format
     */
    [[nodiscard]] std::string extractBearerToken(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Sets Cross-Origin Resource Sharing (CORS) headers on response
     * @param response HTTP response to modify
     * @note Allows requests from any origin (*) with common HTTP methods and headers
     */
    void setCorsHeaders(boost::beast::http::response<boost::beast::http::string_body>& response) const noexcept;

    /**
     * @brief Checks if request has JSON content type
     * @param request HTTP request to check
     * @return bool True if Content-Type is application/json, false otherwise
     */
    [[nodiscard]] bool isJsonContentType(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Safely converts string to integer with default value
     * @param str String to convert
     * @param defaultValue Value to return if conversion fails
     * @return int Converted integer or defaultValue on failure
     */
    [[nodiscard]] int stringToInt(const std::string& str, int defaultValue) const noexcept;
};
}

#endif // IHANDLER_H