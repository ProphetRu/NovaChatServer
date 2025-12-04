#include "AuthHandlers.h"
#include "../models/User.h"
#include "../utils/Validators.h"
#include "../utils/PasswordHasher.h"
#include "../utils/Logger.h"

namespace handlers
{
AuthHandlers::AuthHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept :
    jwtManager_{ std::move(jwtManager) },
    dbManager_{ std::move(dbManager) }
{
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept
{
    try 
    {
        const std::string path{ request.target() };

        if (path == "/api/v1/auth/register" && request.method() == boost::beast::http::verb::post) 
        {
            return handleRegister(request);
        }
        if (path == "/api/v1/auth/login" && request.method() == boost::beast::http::verb::post) 
        {
            return handleLogin(request);
        }
        if (path == "/api/v1/auth/refresh" && request.method() == boost::beast::http::verb::post) 
        {
            return handleRefresh(request);
        }
        if (path == "/api/v1/auth/logout" && request.method() == boost::beast::http::verb::post) 
        {
            return handleLogout(request);
        }
        if (path == "/api/v1/auth/password" && request.method() == boost::beast::http::verb::put) 
        {
            return handleChangePassword(request);
        }
        if (path == "/api/v1/auth/account" && request.method() == boost::beast::http::verb::delete_) 
        {
            return handleDeleteAccount(request);
        }
        
        return createErrorResponse(boost::beast::http::status::not_found, "ENDPOINT_NOT_FOUND", "Endpoint not found");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error in AuthHandlers: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "INTERNAL_ERROR", "Internal server error");
    }
}

std::vector<boost::beast::http::verb> AuthHandlers::getSupportedMethods() const noexcept
{
    return { boost::beast::http::verb::post, boost::beast::http::verb::put, boost::beast::http::verb::delete_ };
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleRegister(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    if (!isJsonContentType(request)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_CONTENT_TYPE", "Content-Type must be application/json");
    }

    nlohmann::json jsonBody{};
    if (!isJsonBodyValid(request.body(), jsonBody)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_JSON", "Invalid JSON body");
    }

    if (!jsonBody.contains("login") || !jsonBody.contains("password")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_FIELDS", "Login and password are required");
    }

    const auto login{ jsonBody["login"].get<std::string>() };
    if (!utils::Validators::isLoginValid(login)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_LOGIN", "Login must be 3-50 characters and contain only letters, numbers and underscores");
    }

    if (isUserExists(login))
    {
        return createErrorResponse(boost::beast::http::status::conflict, "LOGIN_EXISTS", "User with this login already exists");
    }

    const auto password{ jsonBody["password"].get<std::string>() };
    if (!utils::Validators::isPasswordValid(password)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_PASSWORD", "Password must be at least 6 characters and contain at least one letter and one digit");
    }

    try 
    {
        // creating a user
        const auto user{ models::User::createFromCredentials(login, password) };

        // Saving to the database
        dbManager_->executeQuery(user.generateInsertSql());

        nlohmann::json responseData{};
        responseData["user_id"] = user.getUserId();
        responseData["login"] = user.getLogin();

        LOG_INFO("User registered successfully: " + login);
        return createSuccessResponse(responseData, boost::beast::http::status::created, "User registered successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Registration failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "REGISTRATION_FAILED", "Failed to create user");
    }
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleLogin(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    if (!isJsonContentType(request)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_CONTENT_TYPE", "Content-Type must be application/json");
    }

    nlohmann::json jsonBody{};
    if (!isJsonBodyValid(request.body(), jsonBody)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_JSON", "Invalid JSON body");
    }

    if (!jsonBody.contains("login") || !jsonBody.contains("password")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_FIELDS", "Login and password are required");
    }

    auto login{ jsonBody["login"].get<std::string>() };
    auto password{ jsonBody["password"].get<std::string>() };

    try 
    {
        auto result{ dbManager_->executeQuery("SELECT user_id, password_hash FROM users WHERE login = '" + login + "'") };

        if (result.empty()) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_CREDENTIALS", "Invalid login or password");
        }

        auto userId{ result[0]["user_id"].as<std::string>() };
        auto passwordHash{ result[0]["password_hash"].as<std::string>() };

        if (!utils::PasswordHasher::isPasswordValid(password, passwordHash)) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_CREDENTIALS", "Invalid login or password");
        }

        // token generation
        auto accessToken{ jwtManager_->generateAccessToken(userId, login) };
        auto refreshToken{ jwtManager_->generateRefreshToken(userId) };

        // saving a refresh token to the database
        if (!storeRefreshToken(userId, refreshToken)) 
        {
            return createErrorResponse(boost::beast::http::status::internal_server_error, "TOKEN_STORAGE_FAILED", "Failed to store refresh token");
        }

        nlohmann::json responseData{};
        responseData["access_token"] = accessToken;
        responseData["refresh_token"] = refreshToken;
        responseData["token_type"] = "Bearer";
        responseData["expires_in"] = 900; // 15 minutes
        responseData["user_id"] = userId;
        responseData["login"] = login;

        LOG_INFO("User logged in successfully: " + login);
        return createSuccessResponse(responseData, boost::beast::http::status::ok, "Login successful");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Login failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "LOGIN_FAILED", "Login failed");
    }
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleRefresh(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    if (!isJsonContentType(request)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_CONTENT_TYPE", "Content-Type must be application/json");
    }

    nlohmann::json jsonBody{};
    if (!isJsonBodyValid(request.body(), jsonBody)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_JSON", "Invalid JSON body");
    }

    if (!jsonBody.contains("refresh_token")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_TOKEN", "Refresh token is required");
    }

    try 
    {
        auto refreshToken{ jsonBody["refresh_token"].get<std::string>() };

        // verification of refresh token
        auto payload{ jwtManager_->verifyAndDecode(refreshToken) };

        if (!payload.isValid || !payload.isRefreshToken()) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_REFRESH_TOKEN", "Refresh token is invalid");
        }

        // checking the existence of a refresh token in the database
        auto result{ dbManager_->executeQuery(
            "SELECT user_id FROM refresh_tokens WHERE token_hash = '" +
            utils::PasswordHasher::sha256(refreshToken) + "' AND expires_at > NOW()"
        ) };

        if (result.empty()) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_REFRESH_TOKEN", "Refresh token not found or expired");
        }

        // obtaining a user login
        auto userResult{ dbManager_->executeQuery("SELECT login FROM users WHERE user_id = '" + payload.userID + "'") };

        if (userResult.empty()) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "USER_NOT_FOUND", "User not found");
        }

        auto login{ userResult[0]["login"].as<std::string>() };

        // generation of new tokens
        auto newAccessToken{ jwtManager_->generateAccessToken(payload.userID, login) };
        auto newRefreshToken{ jwtManager_->generateRefreshToken(payload.userID) };

        // updating the refresh token in the database
        if (!storeRefreshToken(payload.userID, newRefreshToken)) 
        {
            return createErrorResponse(boost::beast::http::status::internal_server_error, "TOKEN_STORAGE_FAILED", "Failed to store refresh token");
        }

        // deleting the old refresh token
        if (!invalidateRefreshToken(refreshToken))
        {
            LOG_WARNING(std::format("Failed to invalidate old refresh token for user: {}", payload.userID));
        }

        nlohmann::json responseData{};
        responseData["access_token"] = newAccessToken;
        responseData["refresh_token"] = newRefreshToken;
        responseData["token_type"] = "Bearer";
        responseData["expires_in"] = 900; // 15 minutes
        responseData["user_id"] = payload.userID;

        LOG_DEBUG("Tokens refreshed for user: " + payload.userID);
        return createSuccessResponse(responseData, boost::beast::http::status::ok, "Tokens refreshed successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Token refresh failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::unauthorized, "REFRESH_FAILED", "Token refresh failed");
    }
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleLogout(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    if (!isJsonContentType(request)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_CONTENT_TYPE", "Content-Type must be application/json");
    }

    nlohmann::json jsonBody{};
    if (!isJsonBodyValid(request.body(), jsonBody)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_JSON", "Invalid JSON body");
    }

    if (!jsonBody.contains("refresh_token")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_TOKEN", "Refresh token is required");
    }

    try 
    {
        const auto refreshToken{ jsonBody["refresh_token"].get<std::string>() };

        // access token verification
        std::string userId;
        if (!isAuthTokenValid(accessToken, userId)) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
        }

        // adding an access token to the blacklist
        jwtManager_->addTokenToBlacklist(accessToken);

        // removing a refresh token from the database
        if (!invalidateRefreshToken(refreshToken)) 
        {
            LOG_WARNING("Failed to invalidate refresh token for user: " + userId);
        }

        LOG_INFO("User logged out successfully: " + userId);
        return createSuccessResponse({}, boost::beast::http::status::ok, "Successfully logged out");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Logout failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "LOGOUT_FAILED", "Logout failed");
    }
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleChangePassword(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    if (!isJsonContentType(request)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_CONTENT_TYPE", "Content-Type must be application/json");
    }

    nlohmann::json jsonBody{};
    if (!isJsonBodyValid(request.body(), jsonBody)) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_JSON", "Invalid JSON body");
    }

    if (!jsonBody.contains("old_password") || !jsonBody.contains("new_password")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_FIELDS", "Old password and new password are required");
    }

    try 
    {
        const auto oldPassword{ jsonBody["old_password"].get<std::string>() };
        const auto newPassword{ jsonBody["new_password"].get<std::string>() };

        // access token verification
        std::string userId;
        if (!isAuthTokenValid(accessToken, userId)) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
        }

        // checking the current password
        if (!isCurrentPasswordValid(userId, oldPassword)) 
        {
            return createErrorResponse(boost::beast::http::status::forbidden, "INVALID_PASSWORD", "Current password is incorrect");
        }

        // validating a new password
        if (!utils::Validators::isPasswordValid(newPassword)) 
        {
            return createErrorResponse(boost::beast::http::status::bad_request, "INVALID_PASSWORD", "New password must be at least 6 characters and contain at least one letter and one digit");
        }

        // update password
        const auto newPasswordHash{ utils::PasswordHasher::hashPassword(newPassword) };
        auto result{ dbManager_->executeQuery(
            "UPDATE users SET password_hash = '" + newPasswordHash +
            "' WHERE user_id = '" + userId + "'"
        ) };

        LOG_INFO("Password changed successfully for user: " + userId);
        return createSuccessResponse({}, boost::beast::http::status::ok, "Password changed successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Password change failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "PASSWORD_CHANGE_FAILED", "Password change failed");
    }
}

boost::beast::http::response<boost::beast::http::string_body> AuthHandlers::handleDeleteAccount(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    try 
    {
        // Access token verification
        std::string userId;
        if (!isAuthTokenValid(accessToken, userId)) 
        {
            return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
        }

        // Deleting a user (cascading deletion of messages and tokens)
        auto result{ dbManager_->executeQuery("DELETE FROM users WHERE user_id = '" + userId + "'") };

        // Adding an access token to the blacklist
        jwtManager_->addTokenToBlacklist(accessToken);

        LOG_INFO("Account deleted successfully: " + userId);
        return createSuccessResponse({}, boost::beast::http::status::ok, "Account deleted successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Account deletion failed: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "ACCOUNT_DELETION_FAILED", "Account deletion failed");
    }
}

bool AuthHandlers::isAuthTokenValid(const std::string& token, std::string& userId) const noexcept
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

bool AuthHandlers::isUserExists(const std::string& login) const noexcept
{
    try 
    {
        const auto result{ dbManager_->executeQuery("SELECT user_id FROM users WHERE login = '" + login + "'") };
        return !result.empty();
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error checking user existence: " + std::string{ e.what() });
        return false;
    }
}

bool AuthHandlers::isCurrentPasswordValid(const std::string& userId, const std::string& password) const noexcept
{
    try 
    {
        const auto result{ dbManager_->executeQuery("SELECT password_hash FROM users WHERE user_id = '" + userId + "'") };
        if (result.empty()) 
        {
            return false;
        }

        const auto passwordHash{ result[0]["password_hash"].as<std::string>() };
        return utils::PasswordHasher::isPasswordValid(password, passwordHash);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error verifying current password: " + std::string{ e.what() });
        return false;
    }
}

bool AuthHandlers::storeRefreshToken(const std::string& userId, const std::string& refreshToken) const noexcept
{
    try 
    {
        const auto tokenHash{ utils::PasswordHasher::sha256(refreshToken) };

        // getting the expiration time from a token
        const auto expiry{ jwtManager_->getTokenExpiry(refreshToken) };
        const auto expiryTimeT{ std::chrono::system_clock::to_time_t(expiry) };

        std::stringstream ss{};
        std::tm tmBuf{};

#ifdef _WIN32
        localtime_s(&tmBuf, &expiryTimeT);
#else
        localtime_r(&now, &tmBuf);
#endif // endif _WIN32

        ss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M:%S");
        const auto expiryStr{ ss.str() };

        auto result{ dbManager_->executeQuery(
            "INSERT INTO refresh_tokens (user_id, token_hash, expires_at) VALUES ('" +
            userId + "', '" + tokenHash + "', '" + expiryStr + "')"
        ) };

        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error storing refresh token: " + std::string{ e.what() });
        return false;
    }
}

bool AuthHandlers::invalidateRefreshToken(const std::string& refreshToken) const noexcept
{
    try 
    {
        const auto result{ dbManager_->executeQuery("DELETE FROM refresh_tokens WHERE token_hash = '" + utils::PasswordHasher::sha256(refreshToken) + "'") };
        return true;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error invalidating refresh token: " + std::string{ e.what() });
        return false;
    }
}
}
