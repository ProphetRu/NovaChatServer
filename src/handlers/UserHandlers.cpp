#include "UserHandlers.h"
#include "../utils/Logger.h"

namespace handlers
{
constexpr auto PAGE_DEFAULT{ 1 };
constexpr auto LIMIT_DEFAULT{ 50 };

UserHandlers::UserHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept :
    jwtManager_{ std::move(jwtManager) },
    dbManager_{ std::move(dbManager) }
{
}

boost::beast::http::response<boost::beast::http::string_body> UserHandlers::handleRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept
{
    try 
    {
        const std::string path{ request.target() };

        if (path.find("/api/v1/users/search") == 0 && request.method() == boost::beast::http::verb::get)
        {
            return handleSearchUsers(request);
        }
        if (path.find("/api/v1/users") == 0 && request.method() == boost::beast::http::verb::get)
        {
            return handleGetUsers(request);
        }
        
        return createErrorResponse(boost::beast::http::status::not_found, "ENDPOINT_NOT_FOUND", "Endpoint not found");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error in UserHandlers: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "INTERNAL_ERROR", "Internal server error");
    }
}

std::vector<boost::beast::http::verb> UserHandlers::getSupportedMethods() const noexcept
{
    return { boost::beast::http::verb::get };
}

boost::beast::http::response<boost::beast::http::string_body> UserHandlers::handleGetUsers(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    if (std::string userId; !isAuthTokenValid(accessToken, userId)) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
    }

    // parsing request parameters
    std::string target{ request.target() };
    auto queryPos{ target.find('?') };
    auto queryString{ (queryPos != std::string::npos) ? target.substr(queryPos + 1) : "" };

    auto page{ PAGE_DEFAULT };
    auto limit{ LIMIT_DEFAULT };
    std::string search;

    // parsing query parameters
    if (!queryString.empty()) 
    {
        std::istringstream iss{ queryString };
        std::string token;

        while (std::getline(iss, token, '&')) 
        {
	        if (auto eqPos{ token.find('=') }; eqPos != std::string::npos) 
            {
                auto key{ token.substr(0, eqPos) };
                auto value{ token.substr(eqPos + 1) };

                if (key == "page") 
                {
                    page = std::max(1, stringToInt(value, PAGE_DEFAULT));
                }
                else if (key == "limit") 
                {
                    limit = std::min(100, std::max(1, stringToInt(value, LIMIT_DEFAULT)));
                }
                else if (key == "search") 
                {
                    search = value;
                }
            }
        }
    }

    try 
    {
        auto users{ getUsersPaginated(page, limit, search) };
        auto totalCount{ getTotalUsersCount(search) };
        auto totalPages{ (totalCount + limit - 1) / limit };

        auto usersJson{ nlohmann::json::array() };
        for (const auto& user : users) 
        {
            nlohmann::json userJson{};

            userJson["user_id"] = user.getUserId();
            userJson["login"] = user.getLogin();

            usersJson.emplace_back(userJson);
        }

        nlohmann::json pagination{};
        pagination["page"] = page;
        pagination["limit"] = limit;
        pagination["total_count"] = totalCount;
        pagination["total_pages"] = totalPages;
        pagination["has_next"] = (page < totalPages);
        pagination["has_prev"] = (page > 1);

        nlohmann::json responseData{};
        responseData["users"] = usersJson;
        responseData["pagination"] = pagination;

        return createSuccessResponse(responseData);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to get users: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "GET_USERS_FAILED", "Failed to get users");
    }
}

boost::beast::http::response<boost::beast::http::string_body> UserHandlers::handleSearchUsers(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    if (std::string userId; !isAuthTokenValid(accessToken, userId)) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
    }

    // parsing request parameters
    std::string target{ request.target() };
    auto queryPos{ target.find('?') };
    auto queryString{ (queryPos != std::string::npos) ? target.substr(queryPos + 1) : "" };

    std::string query;
    auto limit{ 20 };

    if (!queryString.empty()) 
    {
        std::istringstream iss{ queryString };
        std::string token;

        while (std::getline(iss, token, '&')) 
        {
	        if (auto eqPos{ token.find('=') }; eqPos != std::string::npos) 
            {
                auto key{ token.substr(0, eqPos) };
                auto value{ token.substr(eqPos + 1) };

                if (key == "query") 
                {
                    query = value;
                }
                else if (key == "limit") 
                {
                    limit = std::min(50, std::max(1, stringToInt(value, 20)));
                }
            }
        }
    }

    if (query.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_QUERY", "Search query is required");
    }

    try 
    {
        auto users{ searchUsers(query, limit) };

        auto usersJson{ nlohmann::json::array() };
        for (const auto& user : users) 
        {
            nlohmann::json userJson{};

            userJson["user_id"] = user.getUserId();
            userJson["login"] = user.getLogin();

            usersJson.emplace_back(userJson);
        }

        nlohmann::json meta{};
        meta["query"] = query;
        meta["count"] = users.size();
        meta["limit"] = limit;

        nlohmann::json responseData{};
        responseData["users"] = usersJson;
        responseData["meta"] = meta;

        return createSuccessResponse(responseData);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to search users: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "SEARCH_FAILED", "Search failed");
    }
}

bool UserHandlers::isAuthTokenValid(const std::string& token, std::string& userId) const noexcept
{
    try 
    {
	    if (const auto payload{ jwtManager_->verifyAndDecode(token) }; payload.isValid && payload.isAccessToken()) 
        {
            userId = payload.userID;
            return true;
        }

        return false;
    }
    catch (const std::exception&) 
    {
        return false;
    }
}

std::vector<models::User> UserHandlers::getUsersPaginated(int page, int limit, const std::string& search) const
{
    std::vector<models::User> users;

    const auto offset{ (page - 1) * limit };

    std::string sql{ "SELECT user_id, login, created_at FROM users" };

    if (!search.empty()) 
    {
        sql += " WHERE login ILIKE '%" + search + "%'";
    }

    sql += " ORDER BY created_at DESC LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset);

    try 
    {
        const auto result{ dbManager_->executeQuery(sql) };

        for (const auto& row : result) 
        {
	        models::User user{};
            nlohmann::json userJson{};

            userJson["user_id"] = row["user_id"].as<std::string>();
            userJson["login"] = row["login"].as<std::string>();
            userJson["created_at"] = row["created_at"].as<std::string>();

            user.fromDatabaseRow(userJson);
            users.emplace_back(user);
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting paginated users: " + std::string{ e.what() });
        throw;
    }

    return users;
}

std::vector<models::User> UserHandlers::searchUsers(const std::string& query, int limit) const
{
    std::vector<models::User> users;

    const auto sql = "SELECT user_id, login, created_at FROM users " +
        std::string("WHERE login ILIKE '%") + query + "%' " +
        "ORDER BY login LIMIT " + std::to_string(limit);

    try 
    {
        const auto result{ dbManager_->executeQuery(sql) };

        for (const auto& row : result) 
        {
	        models::User user{};
            nlohmann::json userJson{};

            userJson["user_id"] = row["user_id"].as<std::string>();
            userJson["login"] = row["login"].as<std::string>();
            userJson["created_at"] = row["created_at"].as<std::string>();

            user.fromDatabaseRow(userJson);
            users.emplace_back(user);
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error searching users: " + std::string{ e.what() });
        throw;
    }

    return users;
}

int UserHandlers::getTotalUsersCount(const std::string& search) const noexcept
{
    std::string sql{ "SELECT COUNT(*) as count FROM users" };

    if (!search.empty()) 
    {
        sql += " WHERE login ILIKE '%" + search + "%'";
    }

    try 
    {
	    if (const auto result{ dbManager_->executeQuery(sql) }; !result.empty()) 
        {
            return result[0]["count"].as<int>();
        }

        return 0;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting users count: " + std::string{ e.what() });
        return 0;
    }
}
}
