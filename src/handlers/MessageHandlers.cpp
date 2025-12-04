#include "MessageHandlers.h"
#include "../models/User.h"
#include "../utils/Logger.h"

namespace handlers
{
constexpr auto LIMIT_DEFAULT{ 50 };

MessageHandlers::MessageHandlers(std::shared_ptr<auth::JWTManager> jwtManager, std::shared_ptr<database::DatabaseManager> dbManager) noexcept :
    jwtManager_{ std::move(jwtManager) },
    dbManager_{ std::move(dbManager) }
{
}

boost::beast::http::response<boost::beast::http::string_body> MessageHandlers::handleRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept
{
    try 
    {
        const std::string path{ request.target() };

        if (path == "/api/v1/messages/send" && request.method() == boost::beast::http::verb::post)
        {
            return handleSendMessage(request);
        }
        if (path == "/api/v1/messages/read" && request.method() == boost::beast::http::verb::post)
        {
            return handleMarkAsRead(request);
        }
        if (path.find("/api/v1/messages") == 0 && request.method() == boost::beast::http::verb::get)
        {
            return handleGetMessages(request);
        }
        
        return createErrorResponse(boost::beast::http::status::not_found, "ENDPOINT_NOT_FOUND", "Endpoint not found");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error in MessageHandlers: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "INTERNAL_ERROR", "Internal server error");
    }
}

std::vector<boost::beast::http::verb> MessageHandlers::getSupportedMethods() const noexcept
{
    return { boost::beast::http::verb::get, boost::beast::http::verb::post };
}

boost::beast::http::response<boost::beast::http::string_body> MessageHandlers::handleSendMessage(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    std::string fromUserId;
    if (!isAuthTokenValid(accessToken, fromUserId)) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
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

    if (!jsonBody.contains("to_login") || !jsonBody.contains("message")) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MISSING_FIELDS", "to_login and message are required");
    }

    auto toLogin{ jsonBody["to_login"].get<std::string>() };
    auto messageText{ jsonBody["message"].get<std::string>() };

    if (messageText.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "EMPTY_MESSAGE", "Message cannot be empty");
    }

    if (messageText.length() > 4096) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "MESSAGE_TOO_LONG", "Message exceeds maximum length of 4096 characters");
    }

    try 
    {
        // obtaining the recipient's ID
        auto toUserId{ getUserIdByLogin(toLogin) };
        if (toUserId.empty()) 
        {
            return createErrorResponse(boost::beast::http::status::not_found, "USER_NOT_FOUND", "Recipient user not found");
        }

        // checking that the user is not sending a message to himself
        if (fromUserId == toUserId) 
        {
            return createErrorResponse(boost::beast::http::status::bad_request, "SELF_MESSAGE", "Cannot send message to yourself");
        }

        // creating and saving a message
        auto message{ models::Message::createMessage(fromUserId, toUserId, messageText) };

        dbManager_->executeQuery(message.generateInsertSql());

        nlohmann::json responseData{};
        responseData["message_id"] = message.getMessageId();
        responseData["sent_at"] = message.getCreatedAt();

        LOG_INFO("Message sent from " + fromUserId + " to " + toUserId);
        return createSuccessResponse(responseData, boost::beast::http::status::created, "Message sent successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to send message: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "MESSAGE_SEND_FAILED", "Failed to send message");
    }
}

boost::beast::http::response<boost::beast::http::string_body> MessageHandlers::handleGetMessages(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    std::string userId;
    if (!isAuthTokenValid(accessToken, userId)) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
    }

    // parsing request parameters
    std::string target{ request.target() };
    auto queryPos{ target.find('?') };
    auto queryString{ (queryPos != std::string::npos) ? target.substr(queryPos + 1) : "" };

    auto unreadOnly{ false };
    std::string afterMessageId;
    std::string beforeMessageId;
    auto limit{ LIMIT_DEFAULT };
    std::string conversationWith;

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

                if (key == "unread_only") 
                {
                    unreadOnly = (value == "true");
                }
                else if (key == "after_message_id") 
                {
                    afterMessageId = value;
                }
                else if (key == "before_message_id") 
                {
                    beforeMessageId = value;
                }
                else if (key == "limit") 
                {
                    limit = std::min(200, std::max(1, stringToInt(value, LIMIT_DEFAULT)));
                }
                else if (key == "conversation_with") 
                {
                    conversationWith = value;
                }
            }
        }
    }

    try 
    {
        auto messages{ getMessagesForUser(userId, unreadOnly, afterMessageId, beforeMessageId, limit, conversationWith) };
        auto unreadCount{ getUnreadMessagesCount(userId) };

        auto messagesJson{ nlohmann::json::array() };
        for (const auto& message : messages) 
        {
            nlohmann::json messageJson{};
            messageJson["message_id"] = message.getMessageId();
            messageJson["from_user_id"] = message.getFromUserId();
            messageJson["to_user_id"] = message.getToUserId();
            messageJson["from_login"] = message.getFromLogin();
            messageJson["to_login"] = message.getToLogin();
            messageJson["message_text"] = message.getMessageText();
            messageJson["timestamp"] = message.getCreatedAt();
            messageJson["is_read"] = message.getIsRead();

            messagesJson.emplace_back(messageJson);
        }

        nlohmann::json meta{};
        meta["total_count"] = messages.size();
        meta["unread_count"] = unreadCount;
        meta["has_more"] = (messages.size() == limit);

        if (!messages.empty()) 
        {
            meta["last_message_id"] = messages.back().getMessageId();
        }

        nlohmann::json responseData{};
        responseData["messages"] = messagesJson;
        responseData["meta"] = meta;

        return createSuccessResponse(responseData);
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to get messages: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "GET_MESSAGES_FAILED", "Failed to get messages");
    }
}

boost::beast::http::response<boost::beast::http::string_body> MessageHandlers::handleMarkAsRead(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
	const auto accessToken{ extractBearerToken(request) };
    if (accessToken.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Access token is required");
    }

    std::string userId;
    if (!isAuthTokenValid(accessToken, userId)) 
    {
        return createErrorResponse(boost::beast::http::status::unauthorized, "INVALID_TOKEN", "Invalid access token");
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

    if (!jsonBody.contains("message_ids") || !jsonBody["message_ids"].is_array()) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "EMPTY_MESSAGE_IDS", "Message IDs array is required");
    }

    const auto messageIds{ jsonBody["message_ids"].get<std::vector<std::string>>() };
    if (messageIds.empty()) 
    {
        return createErrorResponse(boost::beast::http::status::bad_request, "EMPTY_MESSAGE_IDS", "Message IDs array cannot be empty");
    }

    try 
    {
        const auto readCount{ markMessagesAsRead(messageIds, userId) };

        nlohmann::json responseData{};
        responseData["read_count"] = readCount;

        LOG_DEBUG("Marked " + std::to_string(readCount) + " messages as read for user: " + userId);
        return createSuccessResponse(responseData, boost::beast::http::status::ok, "Messages marked as read");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to mark messages as read: " + std::string{ e.what() });
        return createErrorResponse(boost::beast::http::status::internal_server_error, "MARK_READ_FAILED", "Failed to mark messages as read");
    }
}

bool MessageHandlers::isAuthTokenValid(const std::string& token, std::string& userId) const noexcept
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

std::string MessageHandlers::getUserIdByLogin(const std::string& login) const noexcept
{
    try 
    {
	    if (const auto result{ dbManager_->executeQuery("SELECT user_id FROM users WHERE login = '" + login + "'") }; !result.empty()) 
        {
            return result[0]["user_id"].as<std::string>();
        }

        return "";
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting user ID by login: " + std::string{ e.what() });
        return "";
    }
}

std::vector<models::Message> MessageHandlers::getMessagesForUser(const std::string& userId, bool isUnreadOnly, const std::string& afterMessageId, const std::string& beforeMessageId, int limit, const std::string& conversationWith) const
{
    std::vector<models::Message> messages;

    // SQL query with JOIN to get logins immediately
    std::string sql = R"(
            SELECT 
                m.message_id,
                m.from_user_id,
                m.to_user_id,
                m.message_text,
                m.is_read,
                m.created_at,
                from_user.login as from_login,
                to_user.login as to_login
            FROM messages m
            LEFT JOIN users from_user ON m.from_user_id = from_user.user_id
            LEFT JOIN users to_user ON m.to_user_id = to_user.user_id
            WHERE (m.from_user_id = ')" + userId + "' OR m.to_user_id = '" + userId + "')";

    if (isUnreadOnly) 
    {
        sql += " AND m.is_read = FALSE AND m.to_user_id = '" + userId + "'";
    }

    if (!conversationWith.empty()) 
    {
	    if (const auto otherUserId{ getUserIdByLogin(conversationWith) }; !otherUserId.empty()) 
        {
            sql += " AND ((m.from_user_id = '" + userId + "' AND m.to_user_id = '" + otherUserId + "') OR " +
                "(m.from_user_id = '" + otherUserId + "' AND m.to_user_id = '" + userId + "'))";
        }
    }

    if (!afterMessageId.empty()) 
    {
        sql += " AND m.message_id > '" + afterMessageId + "'";
    }

    if (!beforeMessageId.empty()) 
    {
        sql += " AND m.message_id < '" + beforeMessageId + "'";
    }

    sql += " ORDER BY m.created_at DESC LIMIT " + std::to_string(limit);

    try 
    {
        auto result = dbManager_->executeQuery(sql);

        for (const auto& row : result) 
        {
	        models::Message message{};
            nlohmann::json messageJson{};

            messageJson["message_id"] = row["message_id"].as<std::string>();
            messageJson["from_user_id"] = row["from_user_id"].as<std::string>();
            messageJson["to_user_id"] = row["to_user_id"].as<std::string>();
            messageJson["from_login"] = row["from_login"].as<std::string>();
            messageJson["to_login"] = row["to_login"].as<std::string>();
            messageJson["message_text"] = row["message_text"].as<std::string>();
            messageJson["is_read"] = row["is_read"].as<bool>();
            messageJson["created_at"] = row["created_at"].as<std::string>();

            message.fromDatabaseRow(messageJson);

            messages.push_back(message);
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting messages for user: " + std::string{ e.what() });
        throw;
    }

    return messages;
}

int MessageHandlers::markMessagesAsRead(const std::vector<std::string>& messageIds, const std::string& userId) const
{
    if (messageIds.empty()) 
    {
        return 0;
    }

    try 
    {
        std::string messageIdsStr;

        for (auto i : std::ranges::views::iota(0u, messageIds.size()))
        {
            if (i > 0)
            {
                messageIdsStr += ", ";
            }

            messageIdsStr += "'" + messageIds[i] + "'";
        }

        const std::string sql{ "UPDATE messages SET is_read = TRUE WHERE message_id IN (" + messageIdsStr + ") AND to_user_id = '" + userId + "'" };
        auto result{ dbManager_->executeQuery(sql) };
        return static_cast<int>(messageIds.size());
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error marking messages as read: " + std::string{ e.what() });
        throw;
    }
}

int MessageHandlers::getUnreadMessagesCount(const std::string& userId) const noexcept
{
    try 
    {
        const auto result{ dbManager_->executeQuery(
            "SELECT COUNT(*) as count FROM messages WHERE to_user_id = '" +
            userId + "' AND is_read = FALSE"
        ) };

        if (!result.empty()) 
        {
            return result[0]["count"].as<int>();
        }

        return 0;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Error getting unread messages count: " + std::string{ e.what() });
        return 0;
    }
}
}
