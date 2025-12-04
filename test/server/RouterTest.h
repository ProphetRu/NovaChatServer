#ifndef ROUTER_TEST_H
#define ROUTER_TEST_H

#include <gtest/gtest.h>

#include "handlers/IHandler.h"

#include "server/Router.h"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>

namespace server
{
class MockHandler : public handlers::IHandler
{
public:
    ~MockHandler() noexcept override = default;

    boost::beast::http::response<boost::beast::http::string_body> handleRequest(const boost::beast::http::request<boost::beast::http::string_body>& request) noexcept override
	{
        boost::beast::http::response<boost::beast::http::string_body> response{boost::beast::http::status::ok, request.version() };
        response.set(boost::beast::http::field::server, "Test Server");
        response.set(boost::beast::http::field::content_type, "application/json");
        response.body() = R"({"status": "ok"})";
        response.prepare_payload();

        return response;
    }

    std::vector<boost::beast::http::verb> getSupportedMethods() const noexcept override { return { boost::beast::http::verb::get }; }

protected:
    bool isAuthTokenValid(const std::string& token, std::string& userId) const noexcept override { return true; }
};

class RouterTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        router_ = std::make_shared<Router>();
        handler_ = std::make_shared<MockHandler>();
    }

    std::shared_ptr<Router> router_;
    std::shared_ptr<MockHandler> handler_;
};

TEST_F(RouterTest, RegisterValidHandler)
{
    EXPECT_NO_THROW(router_->registerHandler("/api/test", handler_));

    const auto paths{ router_->getRegisteredPaths() };
    EXPECT_FALSE(paths.empty());
    EXPECT_EQ(paths[0], "/api/test");
}

TEST_F(RouterTest, RegisterNullHandler)
{
    EXPECT_THROW(router_->registerHandler("/api/test", nullptr), std::invalid_argument);
}

TEST_F(RouterTest, RegisterDuplicateHandler)
{
    EXPECT_NO_THROW(router_->registerHandler("/api/test", handler_));
    EXPECT_NO_THROW(router_->registerHandler("/api/test", handler_)); // should overwrite with warning
}

TEST_F(RouterTest, FindHandlerExactMatch)
{
    router_->registerHandler("/api/test", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/test");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_NE(foundHandler, nullptr);
    EXPECT_EQ(foundHandler.get(), handler_.get());
}

TEST_F(RouterTest, FindHandlerWithQueryParams)
{
    router_->registerHandler("/api/test", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/test?param=value");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_NE(foundHandler, nullptr);
}

TEST_F(RouterTest, FindHandlerBasePathMatch)
{
    router_->registerHandler("/api", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/v1/users");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_NE(foundHandler, nullptr);
}

TEST_F(RouterTest, FindHandlerNotFound)
{
    router_->registerHandler("/api/test", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/unknown");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_EQ(foundHandler, nullptr);
}

TEST_F(RouterTest, FindHandlerEmptyPath)
{
    router_->registerHandler("/", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_NE(foundHandler, nullptr);
}

TEST_F(RouterTest, FindHandlerNormalizePath)
{
    router_->registerHandler("api/test", handler_); // without leading slash

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/test");

    const auto foundHandler{ router_->findHandler(request) };
    EXPECT_NE(foundHandler, nullptr);
}

TEST_F(RouterTest, HandleNotFound)
{
    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/unknown");
    request.version(11);
    request.keep_alive(true);

    const auto response{ router_->handleNotFound(request) };
    EXPECT_EQ(response.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(response[boost::beast::http::field::content_type], "application/json");
    EXPECT_TRUE(response.keep_alive());
}

TEST_F(RouterTest, GetRegisteredPaths)
{
    router_->registerHandler("/api/test1", handler_);
    router_->registerHandler("/api/test2", handler_);

    const auto paths{ router_->getRegisteredPaths() };
    EXPECT_EQ(paths.size(), 2);
    EXPECT_EQ(paths[0], "/api/test1");
    EXPECT_EQ(paths[1], "/api/test2");
}

TEST_F(RouterTest, RemoveHandler)
{
    router_->registerHandler("/api/test", handler_);

    boost::beast::http::request<boost::beast::http::string_body> request;
    request.target("/api/test");

    EXPECT_NE(router_->findHandler(request), nullptr);

    router_->removeHandler("/api/test");
    EXPECT_EQ(router_->findHandler(request), nullptr);
}

TEST_F(RouterTest, RemoveNonExistentHandler)
{
    EXPECT_NO_THROW(router_->removeHandler("/api/nonexistent"));
}

TEST_F(RouterTest, PathNormalization)
{
    // test various path normalization scenarios
    router_->registerHandler("/api/test/", handler_); // with trailing slash

    boost::beast::http::request<boost::beast::http::string_body> request1;
    request1.target("/api/test");

    boost::beast::http::request<boost::beast::http::string_body> request2;
    request2.target("/api/test/");

    EXPECT_NE(router_->findHandler(request1), nullptr);
    EXPECT_NE(router_->findHandler(request2), nullptr);
}
}

#endif // ROUTER_TEST_H
