#include "Router.h"
#include "../utils/Logger.h"

namespace server
{
constexpr auto SERVER_NAME{ "Nova Chat Server" };

void Router::registerHandler(const std::string& path, std::shared_ptr<handlers::IHandler> handler)
{
    if (!handler) 
    {
        LOG_ERROR("Attempt to register null handler for path: " + path);
        throw std::invalid_argument{ "Handler cannot be null" };
    }

    const auto normalizedPath{ normalizePath(path) };

    std::lock_guard lock{ mutex_ };

    if (handlers_.contains(normalizedPath)) 
    {
        LOG_WARNING("Overwriting existing handler for path: " + normalizedPath);
    }

	handlers_[normalizedPath] = std::move(handler);
    LOG_INFO("Registered handler for path: " + normalizedPath);
}

std::shared_ptr<handlers::IHandler> Router::findHandler(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept
{
    std::string requestPath{ request.target() };

    // remove query parameters from the path
    if (const auto queryPos{ requestPath.find('?') }; queryPos != std::string::npos) 
    {
        requestPath = requestPath.substr(0, queryPos);
    }

    const auto normalizedPath{ normalizePath(requestPath) };
    const auto basePath{ extractBasePath(normalizedPath) };

    std::lock_guard lock{ mutex_ };

    // search for an exact match
    if (const auto exactMatch{ handlers_.find(normalizedPath) }; exactMatch != handlers_.end()) 
    {
        LOG_DEBUG("Found exact handler match for path: " + normalizedPath);
        return exactMatch->second;
    }

    // search by base path (for nested paths)
    if (const auto baseMatch{ handlers_.find(basePath) }; baseMatch != handlers_.end() && isPathMatch(normalizedPath, basePath)) 
    {
        LOG_DEBUG("Found base path handler for: " + normalizedPath + " -> " + basePath);
        return baseMatch->second;
    }

    // search by prefix (for API versions)
    for (const auto& [registeredPath, handler] : handlers_) 
    {
        if (normalizedPath.find(registeredPath) == 0 && isPathMatch(normalizedPath, registeredPath)) 
        {
            LOG_DEBUG("Found prefix handler for: " + normalizedPath + " -> " + registeredPath);
            return handler;
        }
    }

    LOG_DEBUG("No handler found for path: " + normalizedPath);
    return nullptr;
}

boost::beast::http::response<boost::beast::http::string_body> Router::handleNotFound(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const std::string target{ request.target() };
    LOG_WARNING("Endpoint not found: " + target);

    nlohmann::json responseJson;
    responseJson["status"] = "error";
    responseJson["code"] = "ENDPOINT_NOT_FOUND";
    responseJson["message"] = "Endpoint not found: " + target;

    boost::beast::http::response<boost::beast::http::string_body> response{ boost::beast::http::status::not_found, request.version() };
    response.set(boost::beast::http::field::server, SERVER_NAME);
    response.set(boost::beast::http::field::content_type, "application/json");
    response.body() = responseJson.dump(4);
    response.prepare_payload();
    response.keep_alive(request.keep_alive());

    // CORS headers
    response.set(boost::beast::http::field::access_control_allow_origin, "*");

    return response;
}

std::vector<std::string> Router::getRegisteredPaths() noexcept
{
    std::lock_guard lock{ mutex_ };

    std::vector<std::string> paths;
    paths.reserve(handlers_.size());

    for (const auto& path : handlers_ | std::views::keys) 
    {
        paths.emplace_back(path);
    }

    std::ranges::sort(paths);
    return paths;
}

void Router::removeHandler(const std::string& path) noexcept
{
    const auto normalizedPath{ normalizePath(path) };

    std::lock_guard lock{ mutex_ };

    if (const auto it{ handlers_.find(normalizedPath) }; it != handlers_.end()) 
    {
        handlers_.erase(it);
        LOG_INFO("Removed handler for path: " + normalizedPath);
    }
    else 
    {
        LOG_WARNING("Attempt to remove non-existent handler for path: " + normalizedPath);
    }
}

std::string Router::normalizePath(const std::string& path) const noexcept
{
    if (path.empty() || path == "/") 
    {
        return "/";
    }

    auto normalized{ path };

    // make sure the path starts with /
    if (normalized[0] != '/') 
    {
        normalized = "/" + normalized;
    }

    // remove the trailing slash except for the root path
    if (normalized.length() > 1 && normalized.back() == '/') 
    {
        normalized.pop_back();
    }

    return normalized;
}

std::string Router::extractBasePath(const std::string& fullPath) const noexcept
{
    if (fullPath.empty() || fullPath == "/") 
    {
        return "/";
    }

    std::vector<std::string> parts;
    std::istringstream iss{ fullPath };
    std::string part;

    // split the path by /
    while (std::getline(iss, part, '/')) 
    {
        if (!part.empty()) 
        {
            parts.emplace_back(part);
        }
    }

    // return the base path (the first two components for the API)
    if (parts.size() >= 2) 
    {
        return "/" + parts[0] + "/" + parts[1];
    }
    if (parts.size() == 1) 
    {
        return "/" + parts[0];
    }

    return "/";
}

bool Router::isPathMatch(const std::string& requestPath, const std::string& registeredPath) const noexcept
{
    // exact match
    if (requestPath == registeredPath) 
    {
        return true;
    }

    // prefix match for API paths
    if (requestPath.find(registeredPath) == 0) 
    {
        // the next character after the prefix is ​​/ or the end of the line
        if (requestPath.length() == registeredPath.length()) 
        {
            return true;
        }
        if (requestPath[registeredPath.length()] == '/') 
        {
            return true;
        }
    }

    return false;
}
}
