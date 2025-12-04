#include "ConfigManager.h"
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace config
{
constexpr uint16_t MIN_PORT{ 1 };
constexpr uint16_t MAX_PORT{ 65535 };
constexpr int MIN_THREADS{ 1 };
constexpr int MAX_THREADS{ 1024 };
constexpr unsigned int MIN_TOKEN_EXPIRY{ 1 };

using json = nlohmann::json;

ConfigManager::ConfigManager(const std::string& configPath)
{
    if (!std::filesystem::exists(configPath)) 
    {
        throw std::runtime_error{ "Config file not found: " + configPath };
    }

    std::ifstream file{ configPath };
    if (!file.is_open()) 
    {
        throw std::runtime_error{ "Cannot open config file: " + configPath };
    }

    file >> config_;
    file.close();

    validateConfig();
}

void ConfigManager::validateConfig() const
{
    if (config_.is_null()) 
    {
        throw std::runtime_error{ "Config is empty" };
    }

    constexpr std::array<std::string_view, 21> requiredPaths
	{
        "server/address",
        "server/port",
        "server/threads",

        "ssl/certificate_file",
        "ssl/private_key_file",
        "ssl/dh_params_file",

        "database/address",
        "database/port",
        "database/username",
        "database/password",
        "database/db_name",
        "database/max_connections",
        "database/connection_timeout",

        "jwt/secret_key",
        "jwt/access_token_expiry_minutes",
        "jwt/refresh_token_expiry_days",

        "logging/level",
        "logging/access_log",
        "logging/error_log",
        "logging/console_output",
        "logging/log_access",
    };

    for (const auto& path : requiredPaths)
    {
        if (!config_.contains(json::json_pointer("/" + std::string{ path.begin(), path.end() })))
        {
            throw std::runtime_error{ "Missing required field: " + std::string{ path } };
        }
    }

	// server settings validation
    if (getServerAddress().empty())
    {
        throw std::runtime_error{ "Server address cannot be empty" };
    }

    if (const auto server_port{ getServerPort() }; server_port < MIN_PORT || server_port >= MAX_PORT)
    {
        throw std::runtime_error{ "Server port must be between " + std::to_string(MIN_PORT) + " and " + std::to_string(MAX_PORT) };
    }

    if (const auto threads{ getServerThreads() }; threads < MIN_THREADS || threads > MAX_THREADS)
    {
        throw std::runtime_error{ "Server threads must be at least " + std::to_string(MIN_THREADS) };
    }

	// SSL files validation
    if (const auto certFile{ getSSLCertificateFile() }; !std::filesystem::exists(certFile)) 
    {
        throw std::runtime_error{ "SSL certificate file not found: " + certFile };
    }

    if (const auto keyFile{ getSSLPrivateKeyFile() }; !std::filesystem::exists(keyFile)) 
    {
        throw std::runtime_error{ "SSL private key file not found: " + keyFile };
    }

    if (const auto dhFile{ getSSLDHParamsFile() }; !std::filesystem::exists(dhFile)) 
    {
        throw std::runtime_error{ "SSL DH params file not found: " + dhFile };
    }

	// database settings validation
    if (getDatabaseAddress().empty())
    {
        throw std::runtime_error{ "Database address cannot be empty" };
    }

    if (const auto db_port{ getDatabasePort() }; db_port < MIN_PORT || db_port >= MAX_PORT)
    {
        throw std::runtime_error{ "Database port must be between " + std::to_string(MIN_PORT) + " and " + std::to_string(MAX_PORT) };
    }

    if (getDatabaseUsername().empty())
    {
        throw std::runtime_error{ "Database username cannot be empty" };
    }

    if (getDatabasePassword().empty())
    {
        throw std::runtime_error{ "Database password cannot be empty" };
    }

    if (getDatabaseDBName().empty())
    {
        throw std::runtime_error{ "Database name cannot be empty" };
    }

    if (getDatabaseMaxConnections() == 0)
    {
        throw std::runtime_error{ "Database max connections must be at least 1" };
    }

    if (getDatabaseConnectionTimeout() == 0)
    {
        throw std::runtime_error{ "Database connection timeout must be at least 1" };
    }

	// JWT settings validation
    if (getJWTSecretKey().empty())
    {
        throw std::runtime_error{ "JWT secret key cannot be empty" };
    }

    if (getJWTAccessTokenExpiryMinutes() < MIN_TOKEN_EXPIRY) 
    {
        throw std::runtime_error{ "JWT access token expiry must be at least 1 minute" };
    }

    if (getJWTRefreshTokenExpiryDays() < MIN_TOKEN_EXPIRY)
    {
        throw std::runtime_error{ "JWT refresh token expiry must be at least 1 day" };
    }

	// logging settings validation
    if (getLoggingLevel().empty())
    {
        throw std::runtime_error{ "Logging level cannot be empty" };
    }

    if (getAccessLogPath().empty())
    {
        throw std::runtime_error{ "Access log path cannot be empty" };
    }

    if (getErrorLogPath().empty())
    {
        throw std::runtime_error{ "Error log path cannot be empty" };
    }
}

template<typename T>
T ConfigManager::getValue(const std::string& path, const T& defaultValue) const noexcept
{
    try 
    {
	    if (const auto ptr{ json::json_pointer("/" + path) }; config_.contains(ptr)) 
        {
            return config_.at(ptr).get<T>();
        }

        return defaultValue;
    }
    catch (const json::exception&) 
    {
        return defaultValue;
    }
}

std::string ConfigManager::getServerAddress() const noexcept
{
    return getValue<std::string>("server/address");
}

uint16_t ConfigManager::getServerPort() const noexcept
{
    return getValue<uint16_t>("server/port");
}

int ConfigManager::getServerThreads() const noexcept
{
    return getValue<int>("server/threads");
}

std::string ConfigManager::getSSLCertificateFile() const noexcept
{
    return getValue<std::string>("ssl/certificate_file");
}

std::string ConfigManager::getSSLPrivateKeyFile() const noexcept
{
    return getValue<std::string>("ssl/private_key_file");
}

std::string ConfigManager::getSSLDHParamsFile() const noexcept
{
    return getValue<std::string>("ssl/dh_params_file");
}

std::string ConfigManager::getDatabaseAddress() const noexcept
{
    return getValue<std::string>("database/address");
}

uint16_t ConfigManager::getDatabasePort() const noexcept
{
    return getValue<uint16_t>("database/port");
}

std::string ConfigManager::getDatabaseUsername() const noexcept
{
    return getValue<std::string>("database/username");
}

std::string ConfigManager::getDatabasePassword() const noexcept
{
    return getValue<std::string>("database/password");
}

std::string ConfigManager::getDatabaseDBName() const noexcept
{
    return getValue<std::string>("database/db_name");
}

unsigned int ConfigManager::getDatabaseMaxConnections() const noexcept
{
    return getValue<unsigned int>("database/max_connections");
}

unsigned int ConfigManager::getDatabaseConnectionTimeout() const noexcept
{
    return getValue<unsigned int>("database/connection_timeout");
}

std::string ConfigManager::getJWTSecretKey() const noexcept
{
    return getValue<std::string>("jwt/secret_key");
}

unsigned int ConfigManager::getJWTAccessTokenExpiryMinutes() const noexcept
{
    return getValue<unsigned int>("jwt/access_token_expiry_minutes");
}

unsigned int ConfigManager::getJWTRefreshTokenExpiryDays() const noexcept
{
    return getValue<unsigned int>("jwt/refresh_token_expiry_days");
}

std::string ConfigManager::getLoggingLevel() const noexcept
{
    return getValue<std::string>("logging/level", "info");
}

std::string ConfigManager::getAccessLogPath() const noexcept
{
    return getValue<std::string>("logging/access_log", "access.log");
}

std::string ConfigManager::getErrorLogPath() const noexcept
{
    return getValue<std::string>("logging/error_log", "error.log");
}

bool ConfigManager::getIsConsoleOutput() const noexcept
{
    return getValue<bool>("logging/console_output", true);
}

bool ConfigManager::getIsLogAccess() const noexcept
{
    return getValue<bool>("logging/log_access", true);
}
}
