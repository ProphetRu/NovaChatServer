#include "Server.h"
#include <filesystem>
#include "../handlers/AuthHandlers.h"
#include "../handlers/UserHandlers.h"
#include "../handlers/MessageHandlers.h"
#include "../utils/Logger.h"

namespace server
{
constexpr std::chrono::seconds GRACEFUL_SHUTDOWN_TIMEOUT{ 30 };
constexpr std::chrono::seconds SHUTDOWN_CHECK_INTERVAL{ 1 };

Server::Server(std::unique_ptr<config::ConfigManager> config, std::shared_ptr<database::DatabaseManager> dbManager, std::shared_ptr<auth::JWTManager> jwtManager) :
    config_{ std::move(config) },
    dbManager_{ std::move(dbManager) },
    jwtManager_{ std::move(jwtManager) },
    ioc_{ std::make_shared<boost::asio::io_context>(config_->getServerThreads()) },
    work_{ boost::asio::make_work_guard(*ioc_) }, // create a work object to prevent io_context from terminating
    sslContext_{ std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12) }
{
    initializeSSL();
    initializeRouter();
    initializeListener();

    LOG_INFO("Server instance created");
}

Server::~Server() noexcept
{
    stop();
}

void Server::start()
{
    if (isRunning_) 
    {
        LOG_WARNING("Server is already running");
        return;
    }

    try 
    {
        LOG_INFO("Starting Server...");

    	listener_->start();

        //work_ = boost::asio::make_work_guard(*ioc_);

        const auto threadCount{ config_->getServerThreads() };
        threads_.reserve(threadCount);

        for (auto _ : std::ranges::views::iota(0, threadCount))
        {
            threads_.emplace_back([this]() noexcept
            {
                try
                {
                    ioc_->run();
                    LOG_DEBUG("IO context thread finished");
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("IO context thread error: " + std::string{ e.what() });
                }
            });
        }

        LOG_INFO("Started " + std::to_string(threadCount) + " worker threads");

        isRunning_ = true;

        LOG_INFO("Server started successfully on " + config_->getServerAddress() + ":" + std::to_string(config_->getServerPort()));
    }
    catch (const std::exception& e) 
    {
        LOG_FATAL("Failed to start server: " + std::string{ e.what() });
        throw;
    }
}

void Server::stop() noexcept
{
    if (!isRunning_)
    {
        LOG_WARNING("Server is already stopped");
        return;
    }

    LOG_INFO("Stopping server...");
    gracefulShutdown();
}

bool Server::isRunning() const noexcept
{
    return isRunning_;
}

void Server::initializeSSL() const
{
    try 
    {
        sslContext_->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::single_dh_use);

        const auto certFile{ config_->getSSLCertificateFile() };
        if (!std::filesystem::exists(certFile)) 
        {
            throw std::runtime_error("SSL certificate file not found: " + certFile);
        }
        sslContext_->use_certificate_chain_file(certFile);

        const auto keyFile{ config_->getSSLPrivateKeyFile() };
        if (!std::filesystem::exists(keyFile)) 
        {
            throw std::runtime_error("SSL private key file not found: " + keyFile);
        }
        sslContext_->use_private_key_file(keyFile, boost::asio::ssl::context::pem);

        const auto dhFile{ config_->getSSLDHParamsFile() };
        if (!std::filesystem::exists(dhFile)) 
        {
            throw std::runtime_error("SSL DH params file not found: " + dhFile);
        }
        sslContext_->use_tmp_dh_file(dhFile);

        LOG_INFO("SSL context initialized successfully");
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("SSL initialization failed: " + std::string{ e.what() });
        throw;
    }
}

void Server::initializeRouter()
{
    try 
    {
	    router_ = std::make_shared<Router>();

        // register handlers
        // auth
	    const auto authHandler{ std::make_shared<handlers::AuthHandlers>(jwtManager_, dbManager_) };
        router_->registerHandler("/api/v1/auth/register", authHandler);
        router_->registerHandler("/api/v1/auth/login", authHandler);
        router_->registerHandler("/api/v1/auth/refresh", authHandler);
        router_->registerHandler("/api/v1/auth/logout", authHandler);
        router_->registerHandler("/api/v1/auth/password", authHandler);
        router_->registerHandler("/api/v1/auth/account", authHandler);

        // users
	    const auto usersHandler{ std::make_shared<handlers::UserHandlers>(jwtManager_, dbManager_) };
        router_->registerHandler("/api/v1/users", usersHandler);
        router_->registerHandler("/api/v1/users/search", usersHandler);

        // messages
	    const auto messagesHandler{ std::make_shared<handlers::MessageHandlers>(jwtManager_, dbManager_) };
        router_->registerHandler("/api/v1/messages", messagesHandler);
        router_->registerHandler("/api/v1/messages/send", messagesHandler);
        router_->registerHandler("/api/v1/messages/read", messagesHandler);

        LOG_INFO("Router initialized with " + std::to_string(router_->getRegisteredPaths().size()) + " routes");

    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Router initialization failed: " + std::string{ e.what() });
        throw;
    }
}

void Server::initializeListener()
{
    try 
    {
        auto address{ boost::asio::ip::make_address(config_->getServerAddress()) };
        auto endpoint{ std::make_unique<boost::asio::ip::tcp::endpoint>(address, config_->getServerPort()) };

        LOG_INFO("Listener initializing on " + endpoint->address().to_string() + ":" + std::to_string(endpoint->port()));

        listener_ = std::make_shared<Listener>(ioc_, std::move(sslContext_), std::move(endpoint), std::move(router_));
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Listener initialization failed: " + std::string{ e.what() });
        throw;
    }
}

void Server::gracefulShutdown() noexcept
{
    LOG_INFO("Stopping listener...");
    if (listener_) 
    {
        listener_->stop();
    }

    LOG_INFO("Waiting for active connections to complete...");
    const auto graceful{ waitForGracefulShutdown() };

    if (!graceful) 
    {
        LOG_WARNING("Graceful shutdown timeout exceeded, forcing shutdown");
    }

    work_.reset();

    LOG_INFO("Stopping IO context...");
    if (!ioc_->stopped()) 
    {
        ioc_->stop();
    }

    LOG_INFO("Waiting for worker threads to finish...");
    for (auto& thread : threads_) 
    {
        if (thread.joinable()) 
        {
            thread.join();
        }
    }
    threads_.clear();

    isRunning_ = false;
    
    LOG_INFO("Server shutdown completed" + std::string{ graceful ? " gracefully" : " forcefully" });
}

bool Server::waitForGracefulShutdown() const noexcept
{
    const auto startTime{ std::chrono::steady_clock::now() };

    ioc_->stop();

    while (std::chrono::steady_clock::now() - startTime < GRACEFUL_SHUTDOWN_TIMEOUT)
    {
        if (ioc_->stopped()) 
        {
            LOG_DEBUG("IO context stopped naturally");
            return true;
        }

        std::this_thread::sleep_for(SHUTDOWN_CHECK_INTERVAL);

        const auto elapsed{ std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - startTime) };
        LOG_DEBUG(std::format("Waiting for shutdown... {}s elapsed", elapsed.count()));
    }

    return false;
}
}
