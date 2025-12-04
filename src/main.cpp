#include <iostream>
#include <string>
#include <memory>
#include <boost/program_options.hpp>
#include "utils/Logger.h"
#include "server/Server.h"

constexpr auto WAIT_TIMEOUT_MS{ 100 };
constexpr auto LOG_TIMEOUT_MIN{ 5 };

struct AppConfig final
{
	std::string configFilePath;
};

[[nodiscard]] AppConfig parseCommandLine(int argc, char* argv[]) noexcept
{
    namespace po = boost::program_options;

    AppConfig appConfig;

    try
    {
        po::options_description desc{ "Nova Chat Server Options" };
        desc.add_options()
            ("help,h", "Show this help message")
            ("config,c", po::value<std::string>(&appConfig.configFilePath)->default_value("config.json"),
                "Path to configuration file")
            ("version,v", "Show version information");

        po::positional_options_description p{};
        p.add("config", 1);

        po::variables_map vm{};
        po::store(po::command_line_parser(argc, argv)
            .options(desc)
            .positional(p)
            .run(),
            vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << "Nova Chat Server - Secure REST API Chat Backend\n\n";
            std::cout << "Usage: " << argv[0] << " [OPTIONS] [CONFIG_FILE]\n\n";
            std::cout << desc << "\n";
            std::cout << "Examples:\n";
            std::cout << "  " << argv[0] << "                    # Use default config.json\n";
            std::cout << "  " << argv[0] << " myconfig.json      # Use custom config file\n";
            std::cout << "  " << argv[0] << " -c production.json # Use -c option\n";
            std::cout << "  " << argv[0] << " --help             # Show this help\n";
            exit(0);
        }

        if (vm.count("version"))
        {
            std::cout << "Nova Chat Server v1.0.0\n";
            exit(0);
        }

        std::cout << "Using configuration file: " << appConfig.configFilePath << std::endl;

    }
    catch (const po::error& e)
    {
        std::cerr << "Error parsing command line: " << e.what() << std::endl;
        std::cerr << "Use --help for usage information" << std::endl;
        exit(1);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        exit(1);
    }

    return appConfig;
}

int main(int argc, char* argv[]) noexcept
{
    try
    {
		// parse command line arguments
        const auto appConfig{ parseCommandLine(argc, argv) };

		// initialize configuration manager
        auto configManager{ std::make_unique<config::ConfigManager>(appConfig.configFilePath) };

		// initialize logger
        utils::Logger::getInstance().initialize(
            configManager->getLoggingLevel(),
            configManager->getAccessLogPath(),
            configManager->getErrorLogPath(),
            configManager->getIsConsoleOutput(),
			configManager->getIsLogAccess()
        );

#ifdef _DEBUG
        LOG_DEBUG("Server is running in debug mode");
        LOG_DEBUG("Configuration");
        LOG_DEBUG("Server.Address: " + configManager->getServerAddress());
        LOG_DEBUG("Server.Port   : " + std::to_string(configManager->getServerPort()));
        LOG_DEBUG("Server.Threads: " + std::to_string(configManager->getServerThreads()));

        LOG_DEBUG("SSL.CertificateFile: " + configManager->getSSLCertificateFile());
        LOG_DEBUG("SSL.PrivateKeyFile : " + configManager->getSSLPrivateKeyFile());
        LOG_DEBUG("SSL.DHParamsFile   : " + configManager->getSSLDHParamsFile());

        LOG_DEBUG("Database.Address          : " + configManager->getDatabaseAddress());
        LOG_DEBUG("Database.Port             : " + std::to_string(configManager->getDatabasePort()));
        LOG_DEBUG("Database.Username         : " + configManager->getDatabaseUsername());
        LOG_DEBUG("Database.Password         : " + configManager->getDatabasePassword());
        LOG_DEBUG("Database.DBName           : " + configManager->getDatabaseDBName());
        LOG_DEBUG("Database.MaxConnections   : " + std::to_string(configManager->getDatabaseMaxConnections()));
        LOG_DEBUG("Database.ConnectionTimeout: " + std::to_string(configManager->getDatabaseConnectionTimeout()));

        LOG_DEBUG("JWT.SecretKey               : " + configManager->getJWTSecretKey());
        LOG_DEBUG("JWT.AccessTokenExpiryMinutes: " + std::to_string(configManager->getJWTAccessTokenExpiryMinutes()));
        LOG_DEBUG("JWT.RefreshTokenExpiryDays  : " + std::to_string(configManager->getJWTRefreshTokenExpiryDays()));

        LOG_DEBUG("LoggingLevel : " + configManager->getLoggingLevel());
        LOG_DEBUG("AccessLogPath: " + configManager->getAccessLogPath());
        LOG_DEBUG("ErrorLogPath : " + configManager->getErrorLogPath());
        LOG_DEBUG("ConsoleOutput: " + (configManager->getIsConsoleOutput() ? std::string{ "true" } : std::string{ "false" }));
        LOG_DEBUG("LogAccess    : " + (configManager->getIsLogAccess() ? std::string{ "true" } : std::string{ "false" }));
#endif

		// initialize database manager
        auto dbManager{ std::make_shared<database::DatabaseManager>(
            configManager->getDatabaseAddress(),
            configManager->getDatabasePort(),
            configManager->getDatabaseUsername(),
            configManager->getDatabasePassword(),
            configManager->getDatabaseDBName(),
            configManager->getDatabaseMaxConnections(),
            configManager->getDatabaseConnectionTimeout()
        ) };

        if (dbManager->healthCheck())
        {
            LOG_INFO("Database connection successful");
        }

        // initialize jwt manager
        auto jwtManager{ std::make_shared<auth::JWTManager>(
            configManager->getJWTSecretKey(),
            configManager->getJWTAccessTokenExpiryMinutes(),
            configManager->getJWTRefreshTokenExpiryDays()
        ) };

		// initialize and start server
        auto server{ std::make_unique<server::Server>(
            std::move(configManager),
            std::move(dbManager),
            std::move(jwtManager)
        ) };

		// start server
		server->start();

		// wait while server is running
        while (server->isRunning()) 
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_TIMEOUT_MS));

            static auto last_stats_time{ std::chrono::steady_clock::now() };
            const auto now{ std::chrono::steady_clock::now() };

            if (std::chrono::duration_cast<std::chrono::minutes>(now - last_stats_time).count() >= LOG_TIMEOUT_MIN) 
            {
                // Statistics logging every LOG_TIMEOUT_MIN minutes
                LOG_INFO("Server is running normally");
                last_stats_time = now;
            }
        }

        server->stop();
    }
    catch (const nlohmann::json::exception& e)
    {
        std::cerr << "Failed parse json config file: " << e.what() << std::endl;
        return -1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "General Failure: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}
