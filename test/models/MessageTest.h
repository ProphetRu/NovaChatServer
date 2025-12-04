#ifndef MESSAGE_MODEL_TEST_H
#define MESSAGE_MODEL_TEST_H

#include <gtest/gtest.h>

#include "models/Message.h"
#include <stdexcept>

namespace models
{
class MessageTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        validFromUserId = "c7da0c43-8ce9-4572-b01b-f10a7dcc47ce";
        validToUserId = "3b9fdda6-dc1b-4544-8b6f-ed39754004d4";
        validText = "Hello, this is a test message!";
        shortText = "Hi";
        longText = std::string(5000, 'a');
        dangerousText = "<script>alert('xss')</script>";
        sameUserId = "c7da0c43-8ce9-4572-b01b-f10a7dcc47ce";
    }

    void TearDown() override {}

    std::string validFromUserId;
    std::string validToUserId;
    std::string validText;
    std::string shortText;
    std::string longText;
    std::string dangerousText;
    std::string sameUserId;
};

TEST_F(MessageTest, ConstructorWithValidParameters)
{
    Message msg(validFromUserId, validToUserId, validText);
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getMessageText(), validText);
    EXPECT_FALSE(msg.getIsRead());
    EXPECT_TRUE(msg.isValid());
}

TEST_F(MessageTest, ConstructorWithSameUserIdsThrowsInValidation)
{
    Message msg(sameUserId, sameUserId, validText);
    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, ConstructorFromValidJson)
{
    const nlohmann::json json
	{
        {"message_id", "msg-123"},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"from_login", "user1"},
        {"to_login", "user2"},
        {"message_text", validText},
        {"is_read", true},
        {"created_at", "2024-01-01 12:00:00"}
    };

    EXPECT_NO_THROW({
        Message msg(json.dump());
        EXPECT_EQ(msg.getFromUserId(), validFromUserId);
        EXPECT_EQ(msg.getToUserId(), validToUserId);
        EXPECT_EQ(msg.getMessageText(), validText);
        EXPECT_TRUE(msg.getIsRead());
    });
}

TEST_F(MessageTest, ConstructorFromInvalidJsonThrows)
{
    const std::string invalidJson{ "{invalid json}" };
    EXPECT_THROW(Message msg(invalidJson), std::invalid_argument);
}

TEST_F(MessageTest, GettersReturnCorrectValues)
{
    Message msg(validFromUserId, validToUserId, validText);
    msg.setMessageId("custom-msg-id");
    msg.setFromLogin("sender");
    msg.setToLogin("receiver");
    msg.setIsRead(true);
    msg.setCreatedAt("2024-01-01 10:00:00");

    EXPECT_EQ(msg.getMessageId(), "custom-msg-id");
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getFromLogin(), "sender");
    EXPECT_EQ(msg.getToLogin(), "receiver");
    EXPECT_EQ(msg.getMessageText(), validText);
    EXPECT_TRUE(msg.getIsRead());
    EXPECT_EQ(msg.getCreatedAt(), "2024-01-01 10:00:00");
}

TEST_F(MessageTest, SetValidMessageText)
{
    Message msg{};
    EXPECT_NO_THROW(msg.setMessageText(validText));
    EXPECT_EQ(msg.getMessageText(), validText);
}

TEST_F(MessageTest, SetShortMessageText)
{
    Message msg{};
    if (utils::Validators::isMessageLengthValid(shortText)) 
    {
        EXPECT_NO_THROW(msg.setMessageText(shortText));
        EXPECT_EQ(msg.getMessageText(), shortText);
    }
    else 
    {
        EXPECT_THROW(msg.setMessageText(shortText), std::invalid_argument);
    }
}

TEST_F(MessageTest, SetLongMessageTextThrows)
{
    Message msg{};
    EXPECT_THROW(msg.setMessageText(longText), std::invalid_argument);
}

TEST_F(MessageTest, SetEmptyMessageTextThrows)
{
    Message msg{};
    EXPECT_THROW(msg.setMessageText(""), std::invalid_argument);
}

TEST_F(MessageTest, SetDangerousMessageTextGetsSanitized)
{
    Message msg{};

    try 
    {
        msg.setMessageText(dangerousText);
    
        EXPECT_NE(msg.getMessageText(), dangerousText);
        EXPECT_FALSE(msg.getMessageText().empty());
    }
    catch (const std::invalid_argument&) 
    {
        SUCCEED();
    }
}

TEST_F(MessageTest, SetUserIdsAndLogins)
{
    Message msg{};

    EXPECT_NO_THROW({
        msg.setFromUserId("from-123");
        msg.setToUserId("to-456");
        msg.setFromLogin("from_user");
        msg.setToLogin("to_user");
    });

    EXPECT_EQ(msg.getFromUserId(), "from-123");
    EXPECT_EQ(msg.getToUserId(), "to-456");
    EXPECT_EQ(msg.getFromLogin(), "from_user");
    EXPECT_EQ(msg.getToLogin(), "to_user");
}

TEST_F(MessageTest, ToJsonIncludesAllFields)
{
    Message msg(validFromUserId, validToUserId, validText);
    msg.setMessageId("json-msg-id");
    msg.setFromLogin("alice");
    msg.setToLogin("bob");
    msg.setIsRead(true);
    msg.setCreatedAt("2024-01-01 11:00:00");

    auto json{ msg.toJson() };

    EXPECT_TRUE(json.contains("message_id"));
    EXPECT_TRUE(json.contains("from_user_id"));
    EXPECT_TRUE(json.contains("to_user_id"));
    EXPECT_TRUE(json.contains("from_login"));
    EXPECT_TRUE(json.contains("to_login"));
    EXPECT_TRUE(json.contains("message_text"));
    EXPECT_TRUE(json.contains("is_read"));
    EXPECT_TRUE(json.contains("created_at"));

    EXPECT_EQ(json["message_id"], "json-msg-id");
    EXPECT_EQ(json["from_user_id"], validFromUserId);
    EXPECT_EQ(json["to_user_id"], validToUserId);
    EXPECT_EQ(json["from_login"], "alice");
    EXPECT_EQ(json["to_login"], "bob");
    EXPECT_EQ(json["message_text"], validText);
    EXPECT_EQ(json["is_read"], true);
    EXPECT_EQ(json["created_at"], "2024-01-01 11:00:00");
}

TEST_F(MessageTest, FromJsonWithValidData)
{
    const nlohmann::json json
	{
        {"message_id", "from-json-id"},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"from_login", "sender"},
        {"to_login", "receiver"},
        {"message_text", validText},
        {"is_read", false},
        {"created_at", "2024-01-01 09:00:00"}
    };

    Message msg{};
    EXPECT_TRUE(msg.fromJson(json));
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getMessageText(), validText);
    EXPECT_TRUE(msg.isValid());
}

TEST_F(MessageTest, FromJsonWithMissingRequiredFieldsFails)
{
    const nlohmann::json json
	{
        {"message_id", "test-id"},
        {"message_text", validText}
    };

    Message msg{};
    EXPECT_FALSE(msg.fromJson(json));
    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, FromJsonWithSameUserIdsFailsValidation)
{
    const nlohmann::json json
    {
        {"from_user_id", sameUserId},
        {"to_user_id", sameUserId},
        {"message_text", validText}
    };

    Message msg{};
    EXPECT_FALSE(msg.fromJson(json));
    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, FromJsonHandlesNullValues)
{
    const nlohmann::json json
	{
        {"message_id", nullptr},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"from_login", nullptr},
        {"message_text", validText},
        {"is_read", nullptr}
    };

    Message msg{};
    EXPECT_TRUE(msg.fromJson(json));
    EXPECT_TRUE(msg.getMessageId().empty());
    EXPECT_TRUE(msg.getFromLogin().empty());
    EXPECT_FALSE(msg.getIsRead());
}

TEST_F(MessageTest, IsValidWithCompleteData)
{
    Message msg(validFromUserId, validToUserId, validText);
    EXPECT_TRUE(msg.isValid());
}

TEST_F(MessageTest, IsInvalidWithEmptyFromUserId)
{
    Message msg{};
    msg.setToUserId(validToUserId);
    msg.setMessageText(validText);

    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, IsInvalidWithEmptyToUserId)
{
    Message msg{};
    msg.setFromUserId(validFromUserId);
    msg.setMessageText(validText);

    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, IsInvalidWithEmptyMessageText)
{
    Message msg{};
    msg.setFromUserId(validFromUserId);
    msg.setToUserId(validToUserId);
    
    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, IsInvalidWithSameUserIds)
{
    Message msg(sameUserId, sameUserId, validText);
    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, IsInvalidWithSameLogins)
{
    Message msg{};
    msg.setFromUserId("user1");
    msg.setToUserId("user2");
    msg.setFromLogin("same_login");
    msg.setToLogin("same_login");
    msg.setMessageText(validText);

    EXPECT_FALSE(msg.isValid());
}

TEST_F(MessageTest, GenerateInsertSqlWithoutMessageId)
{
    Message msg(validFromUserId, validToUserId, validText);

    const auto sql{ msg.generateInsertSql() };

    EXPECT_NE(sql.find("INSERT INTO messages"), std::string::npos);
    EXPECT_NE(sql.find(validFromUserId), std::string::npos);
    EXPECT_NE(sql.find(validToUserId), std::string::npos);
    EXPECT_NE(sql.find(validText), std::string::npos);
    EXPECT_NE(sql.find("FALSE"), std::string::npos);
    EXPECT_EQ(sql.find("message_id"), std::string::npos);
}

TEST_F(MessageTest, GenerateInsertSqlWithMessageId)
{
    Message msg(validFromUserId, validToUserId, validText);
    msg.setMessageId("custom-msg-id");

    const auto sql{ msg.generateInsertSql() };

    EXPECT_NE(sql.find("INSERT INTO messages"), std::string::npos);
    EXPECT_NE(sql.find("message_id"), std::string::npos);
    EXPECT_NE(sql.find("custom-msg-id"), std::string::npos);
}

TEST_F(MessageTest, GenerateInsertSqlWithReadStatus)
{
    Message msg(validFromUserId, validToUserId, validText);
    msg.setIsRead(true);

    const auto sql{ msg.generateInsertSql() };

    EXPECT_NE(sql.find("TRUE"), std::string::npos);
}

TEST_F(MessageTest, GenerateUpdateSqlWithMessageId)
{
    Message msg(validFromUserId, validToUserId, validText);
    msg.setMessageId("update-test-id");
    msg.setIsRead(true);

    EXPECT_NO_THROW({
        std::string sql = msg.generateUpdateSql();
        EXPECT_NE(sql.find("UPDATE messages"), std::string::npos);
        EXPECT_NE(sql.find(validFromUserId), std::string::npos);
        EXPECT_NE(sql.find(validToUserId), std::string::npos);
        EXPECT_NE(sql.find(validText), std::string::npos);
        EXPECT_NE(sql.find("TRUE"), std::string::npos);
        EXPECT_NE(sql.find("update-test-id"), std::string::npos);
    });
}

TEST_F(MessageTest, GenerateUpdateSqlWithoutMessageIdThrows)
{
    Message msg(validFromUserId, validToUserId, validText);

    EXPECT_THROW({
        const auto sql{ msg.generateUpdateSql() };
    }, std::runtime_error);
}

TEST_F(MessageTest, FromDatabaseRowWithCompleteData)
{
    const nlohmann::json row
	{
        {"message_id", "db-msg-123"},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"from_login", "db_sender"},
        {"to_login", "db_receiver"},
        {"message_text", "Database message"},
        {"is_read", true},
        {"created_at", "2024-01-01 08:00:00"}
    };

    Message msg{};
    EXPECT_NO_THROW(msg.fromDatabaseRow(row));

    EXPECT_EQ(msg.getMessageId(), "db-msg-123");
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getFromLogin(), "db_sender");
    EXPECT_EQ(msg.getToLogin(), "db_receiver");
    EXPECT_EQ(msg.getMessageText(), "Database message");
    EXPECT_TRUE(msg.getIsRead());
    EXPECT_EQ(msg.getCreatedAt(), "2024-01-01 08:00:00");
}

TEST_F(MessageTest, FromDatabaseRowWithMissingFields)
{
    const nlohmann::json row
	{
        {"message_id", "partial-msg"},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"message_text", validText}
    };

    Message msg{};
    EXPECT_NO_THROW(msg.fromDatabaseRow(row));

    EXPECT_EQ(msg.getMessageId(), "partial-msg");
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getMessageText(), validText);
    EXPECT_TRUE(msg.getFromLogin().empty());
    EXPECT_TRUE(msg.getToLogin().empty());
    EXPECT_FALSE(msg.getIsRead());
}

TEST_F(MessageTest, FromDatabaseRowWithInvalidDataThrows)
{
    const nlohmann::json row{ "invalid row data" };

    Message msg{};
    EXPECT_THROW(msg.fromDatabaseRow(row), std::exception);
}

TEST_F(MessageTest, FromDatabaseRowWithInvalidMessageThrows)
{
    const nlohmann::json row
	{
        {"from_user_id", "invalid-user-id"},
        {"to_user_id", "invalid-user-id"},
        {"message_text", ""}
    };

    Message msg{};
    EXPECT_THROW(msg.fromDatabaseRow(row), std::runtime_error);
}

TEST_F(MessageTest, MarkAsRead)
{
    Message msg(validFromUserId, validToUserId, validText);
    EXPECT_FALSE(msg.getIsRead());

    msg.markAsRead();
    EXPECT_TRUE(msg.getIsRead());
}

TEST_F(MessageTest, IsFromUser)
{
    Message msg(validFromUserId, validToUserId, validText);

    EXPECT_TRUE(msg.isFromUser(validFromUserId));
    EXPECT_FALSE(msg.isFromUser(validToUserId));
    EXPECT_FALSE(msg.isFromUser("other-user"));
}

TEST_F(MessageTest, IsToUser)
{
    Message msg(validFromUserId, validToUserId, validText);

    EXPECT_TRUE(msg.isToUser(validToUserId));
    EXPECT_FALSE(msg.isToUser(validFromUserId));
    EXPECT_FALSE(msg.isToUser("other-user"));
}

TEST_F(MessageTest, CreateMessageStaticMethod)
{
    const auto msg{ Message::createMessage(validFromUserId, validToUserId, validText) };

    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getMessageText(), validText);
    EXPECT_FALSE(msg.getMessageId().empty());
    EXPECT_TRUE(msg.isValid());
}

TEST_F(MessageTest, FromJsonStringStaticMethod)
{
    const nlohmann::json json
	{
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"message_text", validText}
    };

    EXPECT_NO_THROW({
        const auto msg{ Message::fromJsonString(json.dump()) };
        EXPECT_EQ(msg.getFromUserId(), validFromUserId);
        EXPECT_EQ(msg.getToUserId(), validToUserId);
    });
}

TEST_F(MessageTest, FromJsonStringWithInvalidJsonThrows)
{
    EXPECT_THROW(Message::fromJsonString("invalid json"), std::exception);
}

TEST_F(MessageTest, FromDatabaseStaticMethod)
{
    const nlohmann::json row
    {
        {"message_id", "static-db-id"},
        {"from_user_id", validFromUserId},
        {"to_user_id", validToUserId},
        {"from_login", "static_sender"},
        {"to_login", "static_receiver"},
        {"message_text", "Static database message"},
        {"is_read", false},
        {"created_at", "2024-01-01 07:00:00"}
    };

    const auto msg{ Message::fromDatabase(row) };

    EXPECT_EQ(msg.getMessageId(), "static-db-id");
    EXPECT_EQ(msg.getFromUserId(), validFromUserId);
    EXPECT_EQ(msg.getToUserId(), validToUserId);
    EXPECT_EQ(msg.getFromLogin(), "static_sender");
    EXPECT_EQ(msg.getToLogin(), "static_receiver");
    EXPECT_EQ(msg.getMessageText(), "Static database message");
}

TEST_F(MessageTest, EdgeCaseMaximumLengthMessage)
{
    if (utils::Validators::isMessageLengthValid(longText)) 
    {
        EXPECT_NO_THROW({
            Message msg(validFromUserId, validToUserId, longText);
            EXPECT_TRUE(msg.isValid());
        });
    }
    else 
    {
        EXPECT_THROW(Message msg(validFromUserId, validToUserId, longText), std::invalid_argument);
    }
}

TEST_F(MessageTest, EdgeCaseSpecialCharactersInMessage)
{
    const std::string specialText{ "Hello! @#$%^&*()\nNew line\tTab" };

    EXPECT_NO_THROW({
        Message msg(validFromUserId, validToUserId, specialText);
        EXPECT_EQ(msg.getMessageText(), utils::SecurityUtils::sanitizeUserInput(specialText));
    });
}

TEST_F(MessageTest, EdgeCaseEmptyObject)
{
    Message msg{};
    EXPECT_FALSE(msg.isValid());
    EXPECT_TRUE(msg.getMessageId().empty());
    EXPECT_TRUE(msg.getFromUserId().empty());
    EXPECT_TRUE(msg.getToUserId().empty());
    EXPECT_TRUE(msg.getMessageText().empty());
    EXPECT_FALSE(msg.getIsRead());
}

TEST_F(MessageTest, CopyConstructor)
{
    Message original(validFromUserId, validToUserId, validText);
    original.setMessageId("copy-test-id");
    original.setFromLogin("original_sender");

    Message copy(original);

    EXPECT_EQ(copy.getFromUserId(), original.getFromUserId());
    EXPECT_EQ(copy.getToUserId(), original.getToUserId());
    EXPECT_EQ(copy.getMessageText(), original.getMessageText());
    EXPECT_EQ(copy.getMessageId(), original.getMessageId());
    EXPECT_EQ(copy.getFromLogin(), original.getFromLogin());
}

TEST_F(MessageTest, MoveConstructor)
{
    Message original(validFromUserId, validToUserId, validText);
    original.setMessageId("move-test-id");
    auto originalText{ original.getMessageText() };

    Message moved(std::move(original));

    EXPECT_EQ(moved.getFromUserId(), validFromUserId);
    EXPECT_EQ(moved.getToUserId(), validToUserId);
    EXPECT_EQ(moved.getMessageText(), originalText);
    EXPECT_EQ(moved.getMessageId(), "move-test-id");
}

TEST_F(MessageTest, CopyAssignment)
{
    Message original(validFromUserId, validToUserId, validText);
    original.setMessageId("assign-test-id");
    original.setToLogin("original_receiver");

    Message assigned{};
    assigned = original;

    EXPECT_EQ(assigned.getFromUserId(), original.getFromUserId());
    EXPECT_EQ(assigned.getToUserId(), original.getToUserId());
    EXPECT_EQ(assigned.getMessageText(), original.getMessageText());
    EXPECT_EQ(assigned.getMessageId(), original.getMessageId());
    EXPECT_EQ(assigned.getToLogin(), original.getToLogin());
}

TEST_F(MessageTest, MoveAssignment)
{
    Message original(validFromUserId, validToUserId, validText);
    original.setMessageId("move-assign-test-id");
    std::string originalText = original.getMessageText();

    Message moved{};
    moved = std::move(original);

    EXPECT_EQ(moved.getFromUserId(), validFromUserId);
    EXPECT_EQ(moved.getToUserId(), validToUserId);
    EXPECT_EQ(moved.getMessageText(), originalText);
    EXPECT_EQ(moved.getMessageId(), "move-assign-test-id");
}

TEST_F(MessageTest, TableNameAndPrimaryKey)
{
    Message msg{};

    EXPECT_EQ(msg.getTableName(), "messages");
    EXPECT_EQ(msg.getPrimaryKey(), "message_id");

    msg.setMessageId("primary-key-test");
    EXPECT_EQ(msg.getPrimaryKeyValue(), "primary-key-test");
}
}

#endif // MESSAGE_MODEL_TEST_H
