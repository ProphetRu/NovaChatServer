#include "Message.h"
#include "../utils/Logger.h"
#include "../utils/Validators.h"
#include "../utils/UUIDUtils.h"
#include "../utils/SecurityUtils.h"

namespace models
{
Message::Message(const std::string& fromUserId, const std::string& toUserId, const std::string& text) :
    IModel{}
{
    setFromUserId(fromUserId);
    setToUserId(toUserId);
    setMessageText(text);
}

Message::Message(const std::string& json) :
    IModel{}
{
    try
    {
        if (!fromJson(nlohmann::json::parse(json)))
        {
            throw std::invalid_argument{ "Failed to parse User from JSON" };
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        throw std::invalid_argument{ e.what() };
    }
}

const std::string& Message::getMessageId() const noexcept
{
	return id_;
}

const std::string& Message::getFromUserId() const noexcept
{
	return fromUserID_;
}

const std::string& Message::getToUserId() const noexcept
{
	return toUserID_;
}

const std::string& Message::getFromLogin() const noexcept
{
	return fromLogin_;
}

const std::string& Message::getToLogin() const noexcept
{
    return toLogin_;
}

const std::string& Message::getMessageText() const noexcept
{
	return text_;
}

bool Message::getIsRead() const noexcept
{
	return isRead_;
}

const std::string& Message::getCreatedAt() const noexcept
{
	return createdAt_;
}

void Message::setMessageId(const std::string& id) noexcept
{
	id_ = id;
}

void Message::setFromUserId(const std::string& id) noexcept
{
	fromUserID_ = id;
}

void Message::setToUserId(const std::string& id) noexcept
{
	toUserID_ = id;
}

void Message::setFromLogin(const std::string& login) noexcept
{
    fromLogin_ = login;
}

void Message::setToLogin(const std::string& login) noexcept
{
    toLogin_ = login;
}

void Message::setMessageText(const std::string& text)
{
    if (!utils::Validators::isMessageLengthValid(text)) 
    {
        throw std::invalid_argument{ "Invalid message length" };
    }

    const auto sanitized{ utils::SecurityUtils::sanitizeUserInput(text) };
    if (sanitized.empty()) 
    {
        throw std::invalid_argument{ "Message contains dangerous content" };
    }

    text_ = sanitized;
}

void Message::setIsRead(bool isRead) noexcept
{
	isRead_ = isRead;
}

void Message::setCreatedAt(const std::string& timestamp) noexcept
{
	createdAt_ = timestamp;
}

nlohmann::json Message::toJson() const noexcept
{
    nlohmann::json json{};

    if (!id_.empty()) 
    {
        json["message_id"] = id_;
    }

    json["from_user_id"] = fromUserID_;
    json["to_user_id"] = toUserID_;
    json["from_login"] = fromLogin_;
    json["to_login"] = toLogin_;
    json["message_text"] = text_;
    json["is_read"] = isRead_;
    json["created_at"] = createdAt_;

    return json;
}

bool Message::fromJson(const nlohmann::json& json) noexcept
{
    try 
    {
        if (json.contains("message_id") && !json["message_id"].is_null()) 
        {
            id_ = json["message_id"].get<std::string>();
        }

        if (json.contains("from_user_id") && !json["from_user_id"].is_null()) 
        {
            fromUserID_ = json["from_user_id"].get<std::string>();
        }

        if (json.contains("to_user_id") && !json["to_user_id"].is_null()) 
        {
            toUserID_ = json["to_user_id"].get<std::string>();
        }

        if (json.contains("from_login") && !json["from_login"].is_null())
        {
            fromLogin_ = json["from_login"].get<std::string>();
        }

        if (json.contains("to_login") && !json["to_login"].is_null())
        {
            toLogin_ = json["to_login"].get<std::string>();
        }

        if (json.contains("message_text") && !json["message_text"].is_null()) 
        {
            setMessageText(json["message_text"].get<std::string>());
        }

        if (json.contains("is_read") && !json["is_read"].is_null()) 
        {
            isRead_ = json["is_read"].get<bool>();
        }

        if (json.contains("created_at") && !json["created_at"].is_null()) 
        {
            createdAt_ = json["created_at"].get<std::string>();
        }

        return isValid();

    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to parse Message from JSON: " + std::string{ e.what() });
        return false;
    }
}

bool Message::isValid() const noexcept
{
    if (!utils::Validators::isUUIDValid(fromUserID_)) 
    {
        LOG_ERROR("Message validation failed: invalid from_user_id");
        return false;
    }

    if (!utils::Validators::isUUIDValid(toUserID_)) 
    {
        LOG_ERROR("Message validation failed: invalid to_user_id");
        return false;
    }

    if (fromUserID_ == toUserID_)
    {
        LOG_ERROR("Message validation failed: cannot send message to yourself");
        return false;
    }

    if (!fromLogin_.empty() && !toLogin_.empty())
    {
        if (fromLogin_ == toLogin_)
        {
            LOG_ERROR("Message validation failed: cannot send message to yourself");
            return false;
        }
    }

    if (!utils::Validators::isMessageLengthValid(text_)) 
    {
        LOG_ERROR("Message validation failed: invalid message length");
        return false;
    }

    return true;
}

std::string Message::getTableName() const noexcept
{
	return "messages";
}

std::string Message::getPrimaryKey() const noexcept
{
	return "message_id";
}

std::string Message::getPrimaryKeyValue() const noexcept
{
	return id_;
}

std::string Message::generateInsertSql() const noexcept
{
    std::string sql{ "INSERT INTO messages (from_user_id, to_user_id, message_text" };

    if (!id_.empty()) 
    {
        sql += ", message_id";
    }

    sql += ", is_read) VALUES ('" + fromUserID_ + "', '" + toUserID_ + "', '" + text_ + "'";

    if (!id_.empty()) 
    {
        sql += ", '" + id_ + "'";
    }

    sql += ", " + std::string{ isRead_ ? "TRUE" : "FALSE" } + ")";

    return sql;
}

std::string Message::generateUpdateSql() const
{
    if (id_.empty()) 
    {
        throw std::runtime_error{ "Cannot generate update SQL without id" };
    }

    std::string sql{ "UPDATE messages SET " };
    sql += "from_user_id = '" + fromUserID_ + "'";
    sql += ", to_user_id = '" + toUserID_ + "'";
    sql += ", message_text = '" + text_ + "'";
    sql += ", is_read = " + std::string{ isRead_ ? "TRUE" : "FALSE" };
    sql += " WHERE message_id = '" + id_ + "'";

    return sql;
}

void Message::fromDatabaseRow(const nlohmann::json& row)
{
    try 
    {
        if (row.contains("message_id") && !row["message_id"].is_null()) 
        {
            id_ = row["message_id"].get<std::string>();
        }

        if (row.contains("from_user_id") && !row["from_user_id"].is_null()) 
        {
            fromUserID_ = row["from_user_id"].get<std::string>();
        }

        if (row.contains("to_user_id") && !row["to_user_id"].is_null()) 
        {
            toUserID_ = row["to_user_id"].get<std::string>();
        }

        if (row.contains("from_login") && !row["from_login"].is_null())
        {
            fromLogin_ = row["from_login"].get<std::string>();
        }

        if (row.contains("to_login") && !row["to_login"].is_null())
        {
            toLogin_ = row["to_login"].get<std::string>();
        }

        if (row.contains("message_text") && !row["message_text"].is_null()) 
        {
            text_ = row["message_text"].get<std::string>();
        }

        if (row.contains("is_read") && !row["is_read"].is_null()) 
        {
            isRead_ = row["is_read"].get<bool>();
        }

        if (row.contains("created_at") && !row["created_at"].is_null()) 
        {
            createdAt_ = row["created_at"].get<std::string>();
        }

        if (!isValid())
        {
            throw std::runtime_error{ "Invalid Message data in database row" };
        }
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to parse Message from database row: " + std::string{ e.what() });
        throw;
    }
}

void Message::markAsRead() noexcept
{
	isRead_ = true;
}

bool Message::isFromUser(const std::string& userId) const noexcept
{
	return fromUserID_ == userId;
}

bool Message::isToUser(const std::string& userId) const noexcept
{
	return toUserID_ == userId;
}

Message Message::fromJsonString(const std::string& jsonString)
{
    try 
    {
        return Message{ jsonString };
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to create Message from JSON string: " + std::string{ e.what() });
        throw;
    }
}

Message Message::fromDatabase(const nlohmann::json& row)
{
    Message message{};
    message.fromDatabaseRow(row);
    return message;
}

Message Message::createMessage(const std::string& fromUserId, const std::string& toUserId, const std::string& text)
{
    Message message{};
    message.setFromUserId(fromUserId);
    message.setToUserId(toUserId);
    message.setMessageText(text);
    message.id_ = utils::UUIDUtils::generateUUID();
    return message;
}
}
