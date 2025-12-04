#ifndef USER_MODEL_TEST_H
#define USER_MODEL_TEST_H

#include <gtest/gtest.h>

#include "models/IModel.h"
#include "models/User.h"

#include <stdexcept>

namespace models
{
class UserTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        validLogin = "testuser123";
        validPassword = "SecurePassword123!";
        weakPassword = "123";
        invalidLogin = "ab";
    }

    void TearDown() override {}

    std::string validLogin;
    std::string validPassword;
    std::string weakPassword;
    std::string invalidLogin;
};

TEST_F(UserTest, ConstructorWithValidCredentials)
{
    EXPECT_NO_THROW({
        User user(validLogin, validPassword);
        EXPECT_EQ(user.getLogin(), validLogin);
        EXPECT_FALSE(user.getPasswordHash().empty());
        EXPECT_TRUE(user.isValid());
    });
}

TEST_F(UserTest, ConstructorWithInvalidLoginThrows)
{
    EXPECT_THROW(User user(invalidLogin, validPassword), std::invalid_argument);
}

TEST_F(UserTest, ConstructorWithWeakPasswordThrows)
{
    EXPECT_THROW(User user(validLogin, weakPassword), std::invalid_argument);
}

TEST_F(UserTest, ConstructorFromValidJson)
{
	const nlohmann::json json
    {
        {"user_id", "test-uuid-123"},
        {"login", validLogin},
        {"password", validPassword},
        {"password_hash", "hashed_password_123"},
        {"created_at", "2024-01-01 12:00:00"}
    };

    EXPECT_NO_THROW({
        User user(json.dump());
        EXPECT_EQ(user.getLogin(), validLogin);
        EXPECT_EQ(user.getUserId(), "test-uuid-123");
    });
}

TEST_F(UserTest, ConstructorFromInvalidJsonThrows)
{
    const std::string invalidJson{ "{invalid json}" };
    EXPECT_THROW(User user(invalidJson), std::invalid_argument);
}

TEST_F(UserTest, ConstructorFromEmptyJsonThrows)
{
    const std::string emptyJson{ "{}" };
    EXPECT_THROW(User user(emptyJson), std::invalid_argument);
}

TEST_F(UserTest, GettersReturnCorrectValues)
{
    User user(validLogin, validPassword);
    user.setUserId("custom-id-123");
    user.setCreatedAt("2024-01-01 10:00:00");

    EXPECT_EQ(user.getLogin(), validLogin);
    EXPECT_EQ(user.getUserId(), "custom-id-123");
    EXPECT_EQ(user.getCreatedAt(), "2024-01-01 10:00:00");
    EXPECT_FALSE(user.getPasswordHash().empty());
}

TEST_F(UserTest, SetValidLogin)
{
    User user;
    EXPECT_NO_THROW(user.setLogin(validLogin));
    EXPECT_EQ(user.getLogin(), validLogin);
}

TEST_F(UserTest, SetInvalidLoginThrows)
{
    User user;
    EXPECT_THROW(user.setLogin(invalidLogin), std::invalid_argument);
}

TEST_F(UserTest, SetEmptyLoginThrows)
{
    User user;
    EXPECT_THROW(user.setLogin(""), std::invalid_argument);
}

TEST_F(UserTest, SetValidPassword)
{
    User user{};
    const std::string originalHash{ user.getPasswordHash() };

    EXPECT_NO_THROW(user.setPassword(validPassword));
    EXPECT_NE(user.getPasswordHash(), originalHash);
    EXPECT_FALSE(user.getPasswordHash().empty());
}

TEST_F(UserTest, SetWeakPasswordThrows)
{
    User user{};
    EXPECT_THROW(user.setPassword(weakPassword), std::invalid_argument);
}

TEST_F(UserTest, SetEmptyPasswordThrows)
{
    User user{};
    EXPECT_THROW(user.setPassword(""), std::invalid_argument);
}

TEST_F(UserTest, SetPasswordHashDirectly)
{
    User user{};
    const auto hash{ "direct_hash_value" };

    EXPECT_NO_THROW(user.setPasswordHash(hash));
    EXPECT_EQ(user.getPasswordHash(), hash);
}

TEST_F(UserTest, ToJsonIncludesAllFields)
{
    User user(validLogin, validPassword);
    user.setUserId("json-test-id");

    auto json{ user.toJson() };

    EXPECT_TRUE(json.contains("user_id"));
    EXPECT_TRUE(json.contains("login"));
    EXPECT_EQ(json["user_id"], "json-test-id");
    EXPECT_EQ(json["login"], validLogin);
    
    EXPECT_FALSE(json.contains("password"));
    EXPECT_FALSE(json.contains("password_hash"));
}

TEST_F(UserTest, FromJsonWithValidData)
{
	const nlohmann::json json
	{
        {"user_id", "from-json-id"},
        {"login", validLogin},
        {"password", validPassword},
        {"password_hash", "precomputed_hash"},
        {"created_at", "2024-01-01 12:00:00"}
    };

    User user{};
    EXPECT_TRUE(user.fromJson(json));
    EXPECT_EQ(user.getLogin(), validLogin);
    EXPECT_EQ(user.getUserId(), "from-json-id");
    EXPECT_TRUE(user.isValid());
}

TEST_F(UserTest, FromJsonWithMissingLoginFails)
{
    const nlohmann::json json
	{
        {"user_id", "test-id"},
        {"password", validPassword}
    };

    User user{};
    EXPECT_FALSE(user.fromJson(json));
    EXPECT_FALSE(user.isValid());
}

TEST_F(UserTest, FromJsonWithInvalidDataFails)
{
    const nlohmann::json json
	{
        {"login", invalidLogin},
        {"password", validPassword}
    };

    User user{};
    EXPECT_FALSE(user.fromJson(json));
    EXPECT_FALSE(user.isValid());
}

TEST_F(UserTest, FromJsonHandlesNullValues)
{
    const nlohmann::json json
	{
        {"user_id", nullptr},
        {"login", validLogin},
        {"password", validPassword},
        {"password_hash", nullptr}
    };

    User user{};
    EXPECT_TRUE(user.fromJson(json));
    EXPECT_TRUE(user.getUserId().empty());
    EXPECT_EQ(user.getLogin(), validLogin);
}

TEST_F(UserTest, IsValidWithCompleteData)
{
    User user(validLogin, validPassword);
    user.setUserId("test-id");
    EXPECT_TRUE(user.isValid());
}

TEST_F(UserTest, IsValidWithoutUserId)
{
    User user(validLogin, validPassword);
    EXPECT_TRUE(user.isValid());
}

TEST_F(UserTest, IsInvalidWithEmptyLogin)
{
    User user{};
    user.setPasswordHash("some_hash");
    EXPECT_FALSE(user.isValid());
}

TEST_F(UserTest, IsInvalidWithEmptyPasswordHash) {
    User user;
    user.setLogin(validLogin);
    EXPECT_FALSE(user.isValid());
}

TEST_F(UserTest, GenerateInsertSqlWithoutUserId)
{
    User user(validLogin, validPassword);

    const auto sql{ user.generateInsertSql() };

    EXPECT_NE(sql.find("INSERT INTO users"), std::string::npos);
    EXPECT_NE(sql.find(validLogin), std::string::npos);
    EXPECT_NE(sql.find(user.getPasswordHash()), std::string::npos);
    EXPECT_EQ(sql.find("user_id"), std::string::npos);
}

TEST_F(UserTest, GenerateInsertSqlWithUserId)
{
    User user(validLogin, validPassword);
    user.setUserId("custom-sql-id");

    const auto sql{ user.generateInsertSql() };

    EXPECT_NE(sql.find("INSERT INTO users"), std::string::npos);
    EXPECT_NE(sql.find("user_id"), std::string::npos);
    EXPECT_NE(sql.find("custom-sql-id"), std::string::npos);
}

TEST_F(UserTest, GenerateUpdateSqlWithUserId)
{
    User user(validLogin, validPassword);
    user.setUserId("update-test-id");

    EXPECT_NO_THROW({
        std::string sql = user.generateUpdateSql();
        EXPECT_NE(sql.find("UPDATE users"), std::string::npos);
        EXPECT_NE(sql.find(validLogin), std::string::npos);
        EXPECT_NE(sql.find("update-test-id"), std::string::npos);
    });
}

TEST_F(UserTest, GenerateUpdateSqlWithoutUserIdThrows)
{
    User user(validLogin, validPassword);

    EXPECT_THROW({
        const auto sql{ user.generateUpdateSql() };
    },std::runtime_error);
}

TEST_F(UserTest, FromDatabaseRowWithCompleteData)
{
    const nlohmann::json row
	{
        {"user_id", "db-id-123"},
        {"login", validLogin},
        {"password_hash", "db_password_hash"},
        {"created_at", "2024-01-01 09:00:00"}
    };

    User user{};
    EXPECT_NO_THROW(user.fromDatabaseRow(row));

    EXPECT_EQ(user.getUserId(), "db-id-123");
    EXPECT_EQ(user.getLogin(), validLogin);
    EXPECT_EQ(user.getPasswordHash(), "db_password_hash");
    EXPECT_EQ(user.getCreatedAt(), "2024-01-01 09:00:00");
}

TEST_F(UserTest, FromDatabaseRowWithMissingFields)
{
    const nlohmann::json row
	{
        {"user_id", "partial-id"}
    };

    User user{};
    EXPECT_THROW(user.fromDatabaseRow(row), std::exception);

    EXPECT_EQ(user.getUserId(), "partial-id");
    EXPECT_TRUE(user.getLogin().empty());
    EXPECT_TRUE(user.getPasswordHash().empty());
}

TEST_F(UserTest, FromDatabaseRowWithInvalidDataThrows)
{
    const nlohmann::json row{ "invalid row data" };

    User user{};
    EXPECT_THROW(user.fromDatabaseRow(row), std::exception);
}

TEST_F(UserTest, IsPasswordValidWithCorrectPassword)
{
    User user(validLogin, validPassword);

    EXPECT_TRUE(user.isPasswordValid(validPassword));
}

TEST_F(UserTest, IsPasswordValidWithWrongPassword)
{
    User user(validLogin, validPassword);

    EXPECT_FALSE(user.isPasswordValid("wrong_password"));
}

TEST_F(UserTest, IsPasswordValidWithEmptyPassword)
{
    User user(validLogin, validPassword);

    EXPECT_FALSE(user.isPasswordValid(""));
}

TEST_F(UserTest, CreateFromValidCredentials)
{
    const auto user{ User::createFromCredentials(validLogin, validPassword) };

    EXPECT_EQ(user.getLogin(), validLogin);
    EXPECT_FALSE(user.getUserId().empty());
    EXPECT_FALSE(user.getPasswordHash().empty());
    EXPECT_TRUE(user.isValid());
}

TEST_F(UserTest, FromJsonStringStaticMethod)
{
    const nlohmann::json json
	{
        {"login", validLogin},
        {"password", validPassword}
    };

    EXPECT_NO_THROW({
        const auto user{ User::fromJsonString(json.dump()) };
        EXPECT_EQ(user.getLogin(), validLogin);
    });
}

TEST_F(UserTest, FromJsonStringWithInvalidJsonThrows)
{
    EXPECT_THROW(User::fromJsonString("invalid json"), std::exception);
}

TEST_F(UserTest, FromDatabaseStaticMethod)
{
    const nlohmann::json row
	{
        {"user_id", "static-db-id"},
        {"login", validLogin},
        {"password_hash", "static_hash"},
        {"created_at", "2024-01-01 08:00:00"}
    };

    const auto user{ User::fromDatabase(row) };

    EXPECT_EQ(user.getUserId(), "static-db-id");
    EXPECT_EQ(user.getLogin(), validLogin);
    EXPECT_EQ(user.getPasswordHash(), "static_hash");
}

TEST_F(UserTest, EdgeCaseVeryLongLogin)
{
    const std::string longLogin(1000, 'a');

    if (utils::Validators::isLoginValid(longLogin)) 
    {
        EXPECT_NO_THROW(User user(longLogin, validPassword));
    }
    else 
    {
        EXPECT_THROW(User user(longLogin, validPassword), std::invalid_argument);
    }
}

TEST_F(UserTest, EdgeCaseSpecialCharactersInLogin)
{
    const std::string specialLogin{ "user@domain.com" };

    if (utils::Validators::isLoginValid(specialLogin)) 
    {
        EXPECT_NO_THROW(User user(specialLogin, validPassword));
    }
    else 
    {
        EXPECT_THROW(User user(specialLogin, validPassword), std::invalid_argument);
    }
}

TEST_F(UserTest, EdgeCaseEmptyObject)
{
    User user{};
    EXPECT_FALSE(user.isValid());
    EXPECT_TRUE(user.getUserId().empty());
    EXPECT_TRUE(user.getLogin().empty());
    EXPECT_TRUE(user.getPasswordHash().empty());
}

TEST_F(UserTest, CopyConstructor)
{
    User original(validLogin, validPassword);
    original.setUserId("copy-test-id");

    User copy(original);

    EXPECT_EQ(copy.getLogin(), original.getLogin());
    EXPECT_EQ(copy.getUserId(), original.getUserId());
    EXPECT_EQ(copy.getPasswordHash(), original.getPasswordHash());
}

TEST_F(UserTest, MoveConstructor)
{
    User original(validLogin, validPassword);
    original.setUserId("move-test-id");
    const auto originalHash{ original.getPasswordHash() };

    User moved(std::move(original));

    EXPECT_EQ(moved.getLogin(), validLogin);
    EXPECT_EQ(moved.getUserId(), "move-test-id");
    EXPECT_EQ(moved.getPasswordHash(), originalHash);
}

TEST_F(UserTest, CopyAssignment)
{
    User original(validLogin, validPassword);
    original.setUserId("assign-test-id");

    User assigned{};
    assigned = original;

    EXPECT_EQ(assigned.getLogin(), original.getLogin());
    EXPECT_EQ(assigned.getUserId(), original.getUserId());
    EXPECT_EQ(assigned.getPasswordHash(), original.getPasswordHash());
}

TEST_F(UserTest, MoveAssignment)
{
    User original(validLogin, validPassword);
    original.setUserId("move-assign-test-id");
    std::string originalHash = original.getPasswordHash();

    User moved{};
    moved = std::move(original);

    EXPECT_EQ(moved.getLogin(), validLogin);
    EXPECT_EQ(moved.getUserId(), "move-assign-test-id");
    EXPECT_EQ(moved.getPasswordHash(), originalHash);
}

TEST_F(UserTest, TableNameAndPrimaryKey)
{
    User user{};

    EXPECT_EQ(user.getTableName(), "users");
    EXPECT_EQ(user.getPrimaryKey(), "user_id");

    user.setUserId("primary-key-test");
    EXPECT_EQ(user.getPrimaryKeyValue(), "primary-key-test");
}
}

#endif // USER_MODEL_TEST_H
