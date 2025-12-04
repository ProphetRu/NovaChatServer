#ifndef AUTH_HANDLERS_TEST_H
#define AUTH_HANDLERS_TEST_H

#include <gtest/gtest.h>

#include "handlers/AuthHandlers.h"
#include "auth/JWTManager.h"

#include <algorithm>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>

namespace handlers
{
class AuthHandlersTest : public ::testing::Test
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

        authHandlers_ = std::make_unique<AuthHandlers>(jwtManager_, dbManager_);
    }

    void TearDown() override
    {
        authHandlers_.reset();
        jwtManager_.reset();
    }

    std::shared_ptr<auth::JWTManager> jwtManager_;
    std::shared_ptr<database::DatabaseManager> dbManager_;
    std::unique_ptr<AuthHandlers> authHandlers_;
    std::string secretKey_;
    unsigned int accessExpiryMinutes_{};
    unsigned int refreshExpiryDays_{};
};

TEST_F(AuthHandlersTest, GetSupportedMethods_ReturnsExpectedVerbs)
{
    const auto methods{ authHandlers_->getSupportedMethods() };
    // expect POST, PUT, DELETE are supported
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::post), methods.end());
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::put), methods.end());
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::delete_), methods.end());
}

TEST_F(AuthHandlersTest, HandleRequest_UnknownEndpoint_ReturnsNotFound)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/auth/unknown");

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::not_found);
}

TEST_F(AuthHandlersTest, HandleRegister_InvalidContentType_ReturnsBadRequest)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/auth/register");
    // no Content-Type header
    req.body() = "{}";
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(AuthHandlersTest, HandleLogin_InvalidContentType_ReturnsBadRequest)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/auth/login");
    req.body() = "{}";
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(AuthHandlersTest, HandleRefresh_MissingToken_ReturnsBadRequest)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/auth/refresh");
    req.set("Content-Type", "application/json");
    req.body() = "{}"; // missing refresh_token
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(AuthHandlersTest, HandleLogout_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/auth/logout");
    req.set("Content-Type", "application/json");
    // even if body contains refresh_token, missing Authorization header should cause INVALID_TOKEN
    nlohmann::json body{};
    body["refresh_token"] = "dummy";
    req.body() = body.dump();
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(AuthHandlersTest, HandleChangePassword_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::put);
    req.target("/api/v1/auth/password");
    req.set("Content-Type", "application/json");
    nlohmann::json body{};
    body["old_password"] = "old";
    body["new_password"] = "newPass123";
    req.body() = body.dump();
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(AuthHandlersTest, HandleDeleteAccount_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::delete_);
    req.target("/api/v1/auth/account");

    const auto resp{ authHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(AuthHandlersTest, HandleLogout_WithMalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/auth/logout");
    req.set("Authorization", "Bearer malformed.token.here");
    req.set("Content-Type", "application/json");
    nlohmann::json body{};
    body["refresh_token"] = "dummy";
    req.body() = body.dump();
    req.prepare_payload();

    const auto resp{ authHandlers_->handleRequest(req) };
    // since token is malformed, isAuthTokenValid should return false and handler returns unauthorized
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}
}

#endif // AUTH_HANDLERS_TEST_H
