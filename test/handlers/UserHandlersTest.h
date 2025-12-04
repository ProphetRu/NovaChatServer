#ifndef USER_HANDLERS_TEST_H
#define USER_HANDLERS_TEST_H

#include <gtest/gtest.h>

#include "handlers/UserHandlers.h"
#include "auth/JWTManager.h"

#include <algorithm>
#include <boost/beast/http.hpp>

namespace handlers
{
class UserHandlersTest : public ::testing::Test
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

        userHandlers_ = std::make_unique<UserHandlers>(jwtManager_, dbManager_);
    }

    void TearDown() override
    {
        userHandlers_.reset();
        jwtManager_.reset();
    }

    std::shared_ptr<auth::JWTManager> jwtManager_;
    std::shared_ptr<database::DatabaseManager> dbManager_;
    std::unique_ptr<UserHandlers> userHandlers_;
    std::string secretKey_;
    unsigned int accessExpiryMinutes_{};
    unsigned int refreshExpiryDays_{};
};

TEST_F(UserHandlersTest, GetSupportedMethods_ReturnsGet)
{
    const auto methods{ userHandlers_->getSupportedMethods() };
    EXPECT_NE(std::find(methods.begin(), methods.end(), boost::beast::http::verb::get), methods.end());
}

TEST_F(UserHandlersTest, HandleRequest_UnknownEndpoint_ReturnsNotFound)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::post);
    req.target("/api/v1/users/unknown");

    const auto resp{ userHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::not_found);
}

TEST_F(UserHandlersTest, HandleGetUsers_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/users");

    const auto resp{ userHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(UserHandlersTest, HandleGetUsers_MalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/users");
    req.set("Authorization", "Bearer malformed.token.here");

    const auto resp{ userHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(UserHandlersTest, HandleSearchUsers_MissingAccessToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/users/search?query=test");

    const auto resp{ userHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}

TEST_F(UserHandlersTest, HandleSearchUsers_ValidToken_MissingQueryParam_ReturnsBadRequest)
{
    // generate a valid access token
    const std::string userID = "user123";
    const std::string login = "tester";
    const auto token = jwtManager_->generateAccessToken(userID, login);

    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    // endpoint without query param
    req.target("/api/v1/users/search");
    req.set("Authorization", std::string("Bearer ") + token);

    const auto resp{ userHandlers_->handleRequest(req) };
    // since query is missing, handler should return bad_request
    EXPECT_EQ(resp.result(), boost::beast::http::status::bad_request);
}

TEST_F(UserHandlersTest, HandleSearchUsers_MalformedToken_ReturnsUnauthorized)
{
    boost::beast::http::request<boost::beast::http::string_body> req{};
    req.method(boost::beast::http::verb::get);
    req.target("/api/v1/users/search?query=test");
    req.set("Authorization", "Bearer malformed.token.here");

    const auto resp{ userHandlers_->handleRequest(req) };
    EXPECT_EQ(resp.result(), boost::beast::http::status::unauthorized);
}
}

#endif // USER_HANDLERS_TEST_H
