#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <nlohmann/json.hpp>

namespace config
{
/**
 * @class ConfigManager
 * @brief Manages application configuration from JSON files
 *
 * This class loads, validates, and provides access to configuration settings
 * from a JSON file. It ensures that all required configuration parameters
 * are present and valid before allowing the application to start.
 *
 * @note All getter methods are noexcept and provide default values
 *       when configuration values are missing or invalid.
 */
class ConfigManager final
{
public:
    /**
     * @brief Constructs a ConfigManager and loads configuration from file
     * @param configPath Path to the JSON configuration file
     * @throw std::runtime_error If config file is not found, cannot be opened,
     *        is invalid JSON, or fails validation
     */
    explicit ConfigManager(const std::string& configPath);

    /**
     * @brief Default destructor
     */
    ~ConfigManager() noexcept = default;

    /**
     * @brief Deleted copy constructor
     * @note Configuration manager should not be copied
     */
    ConfigManager(const ConfigManager&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note Configuration manager should not be copied
     */
    ConfigManager& operator=(const ConfigManager&) = delete;

    /**
     * @brief Default move constructor
     * @note Allows moving configuration between instances
     */
    ConfigManager(ConfigManager&&) noexcept = default;

    /**
     * @brief Default move assignment operator
     * @note Allows moving configuration between instances
     */
    ConfigManager& operator=(ConfigManager&&) noexcept = default;

    // Server configuration

    /**
     * @brief Gets the server address from configuration
     * @return std::string Server IP address or hostname
     */
    [[nodiscard]] std::string getServerAddress() const noexcept;

    /**
     * @brief Gets the server port from configuration
     * @return uint16_t Server port number
     */
    [[nodiscard]] uint16_t getServerPort() const noexcept;

    /**
     * @brief Gets the number of server threads from configuration
     * @return int Number of worker threads for the server
     */
    [[nodiscard]] int getServerThreads() const noexcept;

    // SSL/TLS configuration

    /**
     * @brief Gets the SSL certificate file path from configuration
     * @return std::string Path to SSL certificate file
     */
    [[nodiscard]] std::string getSSLCertificateFile() const noexcept;

    /**
     * @brief Gets the SSL private key file path from configuration
     * @return std::string Path to SSL private key file
     */
    [[nodiscard]] std::string getSSLPrivateKeyFile() const noexcept;

    /**
     * @brief Gets the SSL Diffie-Hellman parameters file path from configuration
     * @return std::string Path to SSL DH parameters file
     */
    [[nodiscard]] std::string getSSLDHParamsFile() const noexcept;

    // Database configuration

    /**
     * @brief Gets the database server address from configuration
     * @return std::string Database server IP address or hostname
     */
    [[nodiscard]] std::string getDatabaseAddress() const noexcept;

    /**
     * @brief Gets the database server port from configuration
     * @return uint16_t Database server port number
     */
    [[nodiscard]] uint16_t getDatabasePort() const noexcept;

    /**
     * @brief Gets the database username from configuration
     * @return std::string Database username for authentication
     */
    [[nodiscard]] std::string getDatabaseUsername() const noexcept;

    /**
     * @brief Gets the database password from configuration
     * @return std::string Database password for authentication
     */
    [[nodiscard]] std::string getDatabasePassword() const noexcept;

    /**
     * @brief Gets the database name from configuration
     * @return std::string Name of the database to connect to
     */
    [[nodiscard]] std::string getDatabaseDBName() const noexcept;

    /**
     * @brief Gets the maximum number of database connections from configuration
     * @return unsigned int Maximum size of the database connection pool
     */
    [[nodiscard]] unsigned int getDatabaseMaxConnections() const noexcept;

    /**
     * @brief Gets the database connection timeout from configuration
     * @return unsigned int Connection timeout in seconds
     */
    [[nodiscard]] unsigned int getDatabaseConnectionTimeout() const noexcept;

    // JWT configuration

    /**
     * @brief Gets the JWT secret key from configuration
     * @return std::string Secret key for signing and verifying JWT tokens
     */
    [[nodiscard]] std::string getJWTSecretKey() const noexcept;

    /**
     * @brief Gets the JWT access token expiration time from configuration
     * @return unsigned int Access token validity period in minutes
     */
    [[nodiscard]] unsigned int getJWTAccessTokenExpiryMinutes() const noexcept;

    /**
     * @brief Gets the JWT refresh token expiration time from configuration
     * @return unsigned int Refresh token validity period in days
     */
    [[nodiscard]] unsigned int getJWTRefreshTokenExpiryDays() const noexcept;

    // Logging configuration

    /**
     * @brief Gets the logging level from configuration
     * @return std::string Logging level (debug, info, warn, error, fatal)
     * @note Returns "info" if not specified in configuration
     */
    [[nodiscard]] std::string getLoggingLevel() const noexcept;

    /**
     * @brief Gets the access log file path from configuration
     * @return std::string Path to access log file
     * @note Returns "access.log" if not specified in configuration
     */
    [[nodiscard]] std::string getAccessLogPath() const noexcept;

    /**
     * @brief Gets the error log file path from configuration
     * @return std::string Path to error log file
     * @note Returns "error.log" if not specified in configuration
     */
    [[nodiscard]] std::string getErrorLogPath() const noexcept;

    /**
     * @brief Gets whether console output is enabled from configuration
     * @return bool True if logging to console is enabled
     * @note Returns true if not specified in configuration
     */
    [[nodiscard]] bool getIsConsoleOutput() const noexcept;

    /**
     * @brief Gets whether access logging is enabled from configuration
     * @return bool True if access logging is enabled
     * @note Returns true if not specified in configuration
     */
    [[nodiscard]] bool getIsLogAccess() const noexcept;

private:
    /**
     * @brief Validates the loaded configuration
     * @throw std::runtime_error If any required configuration is missing or invalid
     *
     * Validates all required configuration fields, including:
     * - Server configuration (address, port, threads)
     * - SSL/TLS files existence
     * - Database configuration
     * - JWT configuration
     * - Logging configuration
     */
    void validateConfig() const;

    /**
     * @brief Gets a configuration value with a default fallback
     * @tparam T Type of the configuration value
     * @param path JSON path to the configuration value (e.g., "server/address")
     * @param defaultValue Default value to return if path doesn't exist
     * @return T Configuration value or defaultValue if not found
     *
     * @note This method handles JSON parsing exceptions internally
     *       and returns the defaultValue in case of errors
     */
    template<typename T>
    [[nodiscard]] T getValue(const std::string& path, const T& defaultValue = T()) const noexcept;

private:
    nlohmann::json config_; ///< Internal JSON representation of the configuration
};
}

#endif // CONFIG_MANAGER_H