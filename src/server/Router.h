#ifndef ROUTER_H
#define ROUTER_H

#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include <boost/beast/http.hpp>
#include "../handlers/IHandler.h"

namespace server
{
/**
 * @class Router
 * @brief HTTP request router that maps URL paths to request handlers
 *
 * Manages registration and lookup of request handlers for different URL paths.
 * Supports exact path matching, base path matching for nested routes, and
 * prefix matching for API versioning. Thread-safe for concurrent access.
 *
 * @note Uses normalized paths (trailing slashes removed, leading slash ensured)
 * @see IHandler
 */
class Router final
{
public:
    /**
     * @brief Default constructor
     */
    Router() noexcept = default;

    /**
     * @brief Destructor
     */
    ~Router() noexcept = default;

    /**
     * @brief Deleted copy constructor
     * @note Router should not be copied
     */
    Router(const Router&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note Router should not be copied
     */
    Router& operator=(const Router&) = delete;

    /**
     * @brief Deleted move constructor
     * @note Router should not be moved
     */
    Router(Router&&) noexcept = delete;

    /**
     * @brief Deleted move assignment operator
     * @note Router should not be moved
     */
    Router& operator=(Router&&) noexcept = delete;

    /**
     * @brief Registers a handler for a specific URL path
     * @param path URL path to register handler for
     * @param handler Shared pointer to handler instance
     * @throws std::invalid_argument if handler is null
     * @note Overwrites existing handler for the same path with warning
     */
    void registerHandler(const std::string& path, std::shared_ptr<handlers::IHandler> handler);

    /**
     * @brief Finds appropriate handler for incoming HTTP request
     * @param request HTTP request to find handler for
     * @return std::shared_ptr<handlers::IHandler> Handler if found, nullptr otherwise
     * @note Performs exact match, then base path match, then prefix match
     */
    [[nodiscard]] std::shared_ptr<handlers::IHandler> findHandler(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept;

    /**
     * @brief Creates standardized 404 Not Found response
     * @param request Original HTTP request that triggered the 404
     * @return boost::beast::http::response<boost::beast::http::string_body> HTTP 404 response
     * @note Includes CORS headers and JSON error body
     */
    [[nodiscard]] boost::beast::http::response<boost::beast::http::string_body> handleNotFound(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept;

    /**
     * @brief Retrieves list of all registered URL paths
     * @return std::vector<std::string> Sorted list of registered paths
     * @note Useful for debugging and API documentation
     */
    [[nodiscard]] std::vector<std::string> getRegisteredPaths() noexcept;

    /**
     * @brief Removes handler for specified path
     * @param path URL path to remove handler for
     * @note Safe to call for non-existent paths
     */
    void removeHandler(const std::string& path) noexcept;

private:
    /**
     * @brief Normalizes URL path for consistent matching
     * @param path Raw URL path to normalize
     * @return std::string Normalized path
     * @details Ensures leading slash, removes trailing slash (except root)
     */
    [[nodiscard]] std::string normalizePath(const std::string& path) const noexcept;

    /**
     * @brief Extracts base path from full URL path
     * @param fullPath Full URL path to analyze
     * @return std::string Base path (first two components for API paths)
     */
    [[nodiscard]] std::string extractBasePath(const std::string& fullPath) const noexcept;

    /**
     * @brief Checks if request path matches registered path
     * @param requestPath Normalized request path
     * @param registeredPath Normalized registered path
     * @return bool True if paths match according to routing rules
     * @details Supports exact match and prefix match with path segment boundaries
     */
    [[nodiscard]] bool isPathMatch(const std::string& requestPath, const std::string& registeredPath) const noexcept;

private:
    // path -> handler
    std::unordered_map<std::string, std::shared_ptr<handlers::IHandler>> handlers_; ///< Map of registered paths to handlers
    std::mutex mutex_; ///< Mutex for thread-safe access to handlers map
};
}

#endif // ROUTER_H