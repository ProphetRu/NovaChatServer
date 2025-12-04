#include "User.h"
#include "../utils/Logger.h"
#include "../utils/PasswordHasher.h"
#include "../utils/Validators.h"
#include "../utils/UUIDUtils.h"

namespace models
{
User::User(const std::string& login, const std::string& password) :
	IModel{}
{
	setLogin(login);
	setPassword(password);
}

User::User(const std::string& json) :
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

const std::string& User::getUserId() const noexcept
{
	return id_;
}

const std::string& User::getLogin() const noexcept
{
	return login_;
}

const std::string& User::getPasswordHash() const noexcept
{
	return passwordHash_;
}

const std::string& User::getCreatedAt() const noexcept
{
	return createdAt_;
}

void User::setUserId(const std::string& id) noexcept
{
	id_ = id;
}

void User::setLogin(const std::string& login)
{
    if (!utils::Validators::isLoginValid(login))
    {
        throw std::invalid_argument{ "Invalid login_ format" };
    }

    login_ = login;
}

void User::setPassword(const std::string& password)
{
    if (!utils::Validators::isPasswordValid(password))
    {
        throw std::invalid_argument{ "Invalid password format" };
    }

    passwordHash_ = utils::PasswordHasher::hashPassword(password);
}

void User::setPasswordHash(const std::string& hash) noexcept
{
	passwordHash_ = hash;
}

void User::setCreatedAt(const std::string& timestamp) noexcept
{
	createdAt_ = timestamp;
}

nlohmann::json User::toJson() const noexcept
{
    nlohmann::json json{};

    if (!id_.empty()) 
    {
        json["user_id"] = id_;
    }

    json["login"] = login_;

    return json;
}

bool User::fromJson(const nlohmann::json& json) noexcept
{
    try 
    {
        if (json.contains("user_id") && !json["user_id"].is_null()) 
        {
            id_ = json["user_id"].get<std::string>();
        }

        if (json.contains("login") && !json["login"].is_null()) 
        {
            setLogin(json["login"].get<std::string>());
        }

        if (json.contains("password") && !json["password"].is_null()) 
        {
            setPassword(json["password"].get<std::string>());
        }

        if (json.contains("password_hash") && !json["password_hash"].is_null()) 
        {
            passwordHash_ = json["password_hash"].get<std::string>();
        }

        if (json.contains("created_at") && !json["created_at"].is_null()) {
            createdAt_ = json["created_at"].get<std::string>();
        }

        return isValid();

    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to parse User from JSON: " + std::string{ e.what() });
        return false;
    }
}

bool User::isValid() const noexcept
{
    if (!utils::Validators::isLoginValid(login_)) 
    {
        LOG_ERROR("User validation failed: invalid login");
        return false;
    }

    if (passwordHash_.empty()) 
    {
        LOG_ERROR("User validation failed: password hash is empty");
        return false;
    }

    return true;
}

std::string User::getTableName() const noexcept
{
	return "users";
}

std::string User::getPrimaryKey() const noexcept
{
	return "user_id";
}

std::string User::getPrimaryKeyValue() const noexcept
{
	return id_;
}

std::string User::generateInsertSql() const noexcept
{
    std::string sql{ "INSERT INTO users (login, password_hash" };

    if (!id_.empty()) 
    {
        sql += ", user_id";
    }

    sql += ") VALUES ('" + login_ + "', '" + passwordHash_ + "'";

    if (!id_.empty()) 
    {
        sql += ", '" + id_ + "'";
    }

    sql += ")";

    return sql;
}

std::string User::generateUpdateSql() const
{
    if (id_.empty()) 
    {
        throw std::runtime_error{ "Cannot generate update SQL without id" };
    }

    std::string sql{ "UPDATE users SET " };
    sql += "login = '" + login_ + "'";
    sql += ", password_hash = '" + passwordHash_ + "'";
    sql += " WHERE user_id = '" + id_ + "'";

    return sql;
}

void User::fromDatabaseRow(const nlohmann::json& row)
{
    try 
    {
        if (row.contains("user_id") && !row["user_id"].is_null()) 
        {
            id_ = row["user_id"].get<std::string>();
        }

        if (row.contains("login") && !row["login"].is_null()) 
        {
            login_ = row["login"].get<std::string>();
        }

        if (row.contains("password_hash") && !row["password_hash"].is_null()) 
        {
            passwordHash_ = row["password_hash"].get<std::string>();
        }

        if (row.contains("created_at") && !row["created_at"].is_null()) 
        {
            createdAt_ = row["created_at"].get<std::string>();
        }

        if (id_.empty() || login_.empty() || createdAt_.empty()) 
        {
            throw std::runtime_error{ "Invalid User data in database row" };
		}
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to parse User from database row: " + std::string{ e.what() });
        throw;
    }
}

bool User::isPasswordValid(const std::string& password) const noexcept
{
    return utils::PasswordHasher::isPasswordValid(password, passwordHash_);
}

User User::createFromCredentials(const std::string& login, const std::string& password)
{
    User user{};
    user.setLogin(login);
    user.setPassword(password);
    user.id_ = utils::UUIDUtils::generateUUID();
    return user;
}

User User::fromJsonString(const std::string& jsonString)
{
    try 
    {
        return User{ jsonString };
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to create User from JSON string: " + std::string{ e.what() });
        throw;
    }
}

User User::fromDatabase(const nlohmann::json& row)
{
    User user{};
    user.fromDatabaseRow(row);
    return user;
}
}
