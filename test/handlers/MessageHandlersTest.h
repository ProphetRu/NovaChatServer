#ifndef MESSAGE_HANDLERS_TEST_H
#define MESSAGE_HANDLERS_TEST_H

#include <gtest/gtest.h>

#include "handlers/MessageHandlers.h"
#include "auth/JWTManager.h"

#include <algorithm>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace handlers
{
class MessageHandlersTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        secretKey_ = "test_secret_key_which_is_long_enough_for_tests";
        accessExpiryMinutes_ = 15;
        refreshExpiryDays_ = 7;

        jwtManager_ = std::make_shared<auth::JWTManager>(secretKey_, accessExpiryMinutes_, refreshExpiryDays_);

        // deliberately pass a null database manager for tests that don't touch DB
        dbManager_.reset();

        messageHandlers_ = std::make_unique<MessageHandlers>(jwtManager_, dbManager_);
    }

    void TearDown() override
    {
        messageHandlers_.reset();
        jwtManager_.reset();
    }

    std::shared_ptr<auth::JWTManager> jwtManager_;
    std::shared_ptr<database::DatabaseManager> dbManager_;
    std::unique_ptr<MessageHandlers> messageHandlers_;
    std::string secretKey_;
    unsigned int accessExpiryMinutes_{};
    unsigned int refreshExpiryDays_{};
};

TEST_F(MessageHandlersTest, GetSupportedMethods_ReturnsGetAndPost)
{
    const auto methods{ messageHandlers_->getSupportedMethods() };
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::get), methods.end());
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::post), methods.end());
}

TEST_F(MessageHandlersTest, HandleRequest_UnknownEndpoint_ReturnsNotFound)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::put);
    req.target("/api/v1/messages/unknown");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::not_found);
}

TEST_F(MessageHandlersTest, HandleSendMessage_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleSendMessage_MalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", "Bearer malformed.token.here");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleSendMessage_InvalidContentType_ReturnsBadRequest)
{
    // valid token to pass auth check
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", std::string("Bearer ") + token);
    // missing Content-Type header
    req.body() = R"({"to_login":"recipient","message":"hello"})";
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleSendMessage_InvalidJson_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = "not a json";
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleSendMessage_MissingFields_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = R"({"to_login":"recipient"})"; // missing message
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleSendMessage_EmptyMessage_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = R"({"to_login":"recipient","message":""})";
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleSendMessage_MessageTooLong_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    std::string longMsg(5000, 'x');

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/send");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");

    nlohmann::json j{};
    j["to_login"] = "recipient";
    j["message"] = longMsg;
    req.body() = j.dump();
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleGetMessages_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/messages");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleGetMessages_MalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/messages");
    req.set("Authorization", "Bearer malformed.token.here");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_MalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");
    req.set("Authorization", "Bearer malformed.token.here");

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_InvalidContentType_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");
    req.set("Authorization", std::string("Bearer ") + token);
    // missing Content-Type
    req.body() = R"({"message_ids":["m1"]})";
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_InvalidJson_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = "not json";
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_MissingMessageIds_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = R"({"ids": ["m1"]})"; // wrong field
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(MessageHandlersTest, HandleMarkAsRead_EmptyMessageIds_ReturnsBadRequest)
{
    const auto token{ jwtManager_->generateAccessToken("user1", "sender") };

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/messages/read");
    req.set("Authorization", std::string("Bearer ") + token);
    req.set("Content-Type", "application/json");
    req.body() = R"({"message_ids":[]})"; // empty array should be rejected before DB access
    req.prepare_payload();

    const auto resp{ messageHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}
}

#endif // MESSAGE_HANDLERS_TEST_H
