#ifndef CONFIG_MANAGER_TEST_H
#define CONFIG_MANAGER_TEST_H

#include <gtest/gtest.h>

#include "config/ConfigManager.h"

#include <fstream>
#include <filesystem>
#include <cstdlib>
#include <random>

namespace config
{
class ConfigManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        // create a temporary directory for tests
        testDir_ = "test_config_" + generateRandomString(10);
        std::filesystem::create_directories(testDir_);
        std::filesystem::create_directories(testDir_ + "/sslCerts");

        // creating temporary SSL files
        createTestFile(testDir_ + "/sslCerts/server.crt", "TEST SSL CERTIFICATE");
        createTestFile(testDir_ + "/sslCerts/server.key", "TEST SSL PRIVATE KEY");
        createTestFile(testDir_ + "/sslCerts/dhparams.pem", "TEST DH PARAMS");

        // basic valid config
        baseConfig_ =
        {
            {"server",
			{
                {"address", "0.0.0.0"},
                {"port", 8443},
                {"threads", 4}
            }},
            {"ssl",
			{
                {"certificate_file", testDir_ + "/sslCerts/server.crt"},
                {"private_key_file", testDir_ + "/sslCerts/server.key"},
                {"dh_params_file", testDir_ + "/sslCerts/dhparams.pem"}
            }},
            {"database",
			{
                {"address", "192.168.50.37"},
                {"port", 5432},
                {"username", "chat_user"},
                {"password", "chat_user"},
                {"db_name", "chat_db"},
                {"max_connections", 10},
                {"connection_timeout", 10}
            }},
            {"jwt",
			{
                {"secret_key", "MJ1IdWHzDpT7VfGZQFRScabPuxEs1EEP"},
                {"access_token_expiry_minutes", 15},
                {"refresh_token_expiry_days", 7}
            }},
            {"logging",
			{
                {"level", "debug"},
                {"access_log", "access.log"},
                {"error_log", "error.log"},
                {"console_output", true},
                {"log_access", true}
            }}
        };
    }

    void TearDown() override
	{
        // delete the temporary directory
        std::filesystem::remove_all(testDir_);
    }

    void createConfigFile(const std::string& filename, const nlohmann::json& config)
	{
        std::ofstream file(filename);
        file << config.dump(4);
        file.close();
    }

    void createTestFile(const std::string& filename, const std::string& content)
	{
        std::ofstream file(filename);
        file << content;
        file.close();
    }

    std::string generateRandomString(size_t length)
	{
        static constexpr char alphanum[] =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";

        std::string result;
        result.reserve(length);

        std::random_device rd{};
        std::mt19937 gen{ rd() } ;
        std::uniform_int_distribution<> dis{ 0, sizeof(alphanum) - 2 };

        for (size_t i = 0; i < length; ++i) 
        {
            result += alphanum[dis(gen)];
        }

        return result;
    }

    std::string testDir_;
    nlohmann::json baseConfig_;
};

TEST_F(ConfigManagerTest, Constructor_ValidConfig_Success)
{
    const auto configPath{ testDir_ + "/valid_config.json" };
    createConfigFile(configPath, baseConfig_);

    EXPECT_NO_THROW(
	{
        ConfigManager manager(configPath);

	    EXPECT_EQ(manager.getServerAddress(), "0.0.0.0");
	    EXPECT_EQ(manager.getServerPort(), 8443);
	    EXPECT_EQ(manager.getServerThreads(), 4);

	    EXPECT_EQ(manager.getDatabaseAddress(), "192.168.50.37");
	    EXPECT_EQ(manager.getDatabasePort(), 5432);
	    EXPECT_EQ(manager.getDatabaseUsername(), "chat_user");
	    EXPECT_EQ(manager.getDatabaseDBName(), "chat_db");

	    EXPECT_EQ(manager.getJWTSecretKey(), "MJ1IdWHzDpT7VfGZQFRScabPuxEs1EEP");
	    EXPECT_EQ(manager.getJWTAccessTokenExpiryMinutes(), 15);
	    EXPECT_EQ(manager.getJWTRefreshTokenExpiryDays(), 7);

	    EXPECT_EQ(manager.getLoggingLevel(), "debug");
	    EXPECT_EQ(manager.getAccessLogPath(), "access.log");
	    EXPECT_EQ(manager.getErrorLogPath(), "error.log");
	    EXPECT_TRUE(manager.getIsConsoleOutput());
	    EXPECT_TRUE(manager.getIsLogAccess());
	});
}

TEST_F(ConfigManagerTest, Constructor_ConfigFileNotFound_ThrowsException)
{
    EXPECT_THROW({
        ConfigManager manager("non_existent_config.json");
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Constructor_InvalidJSON_ThrowsException)
{
    const auto configPath{ testDir_ + "/invalid_json.json" };
    std::ofstream file(configPath);
    file << "invalid json content {";
    file.close();

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, nlohmann::json::exception);
}

TEST_F(ConfigManagerTest, Constructor_EmptyConfig_ThrowsException)
{
    const auto configPath{ testDir_ + "/empty_config.json" };
    createConfigFile(configPath, nlohmann::json::object());

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingServerAddress_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"].erase("address");

    const auto configPath{ testDir_ + "/missing_server_address.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingServerPort_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"].erase("port");

    const auto configPath{ testDir_ + "/missing_server_port.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingServerThreads_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"].erase("threads");

    const auto configPath{ testDir_ + "/missing_server_threads.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingSSLCertificateFile_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"].erase("certificate_file");

    const auto configPath{ testDir_ + "/missing_ssl_certificate_file.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingSSLPrivateKeyFile_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"].erase("private_key_file");

    const auto configPath{ testDir_ + "/missing_ssl_private_key_file.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingSSLDHParamsFile_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"].erase("dh_params_file");

    const auto configPath{ testDir_ + "/missing_ssl_dh_params_file.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabaseAddress_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("address");

    const auto configPath{ testDir_ + "/missing_db_address.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabasePort_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("port");

    const auto configPath{ testDir_ + "/missing_db_port.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabaseUsername_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("username");

    const auto configPath{ testDir_ + "/missing_db_username.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabasePassword_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("password");

    const auto configPath{ testDir_ + "/missing_db_password.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabaseDBName_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("db_name");

    const auto configPath{ testDir_ + "/missing_db_db_name.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabaseMaxConnections_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("max_connections");

    const auto configPath{ testDir_ + "/missing_db_max_connections.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingDatabaseConnectionTimeout_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"].erase("connection_timeout");

    const auto configPath{ testDir_ + "/missing_db_connection_timeout.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingJWTSecretKey_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"].erase("secret_key");

    const auto configPath{ testDir_ + "/missing_jwt_secret_key.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingJWTAccessTokenExpiryMinutes_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"].erase("access_token_expiry_minutes");

    const auto configPath{ testDir_ + "/missing_jwt_access_token_expiry_minutes.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingJWTRefreshTokenExpiryDays_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"].erase("refresh_token_expiry_days");

    const auto configPath{ testDir_ + "/missing_jwt_refresh_token_expiry_days.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingLoggingLevel_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"].erase("level");

    const auto configPath{ testDir_ + "/missing_logging_level.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingLoggingAccessLog_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"].erase("access_log");

    const auto configPath{ testDir_ + "/missing_logging_access_log.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingLoggingErrorLog_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"].erase("error_log");

    const auto configPath{ testDir_ + "/missing_logging_error_log.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingLoggingConsoleOutput_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"].erase("console_output");

    const auto configPath{ testDir_ + "/missing_logging_console_output.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_MissingLoggingLogAccess_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"].erase("log_access");

    const auto configPath{ testDir_ + "/missing_logging_log_access.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyServerAddress_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"]["address"] = "";

    const auto configPath{ testDir_ + "/empty_server_address.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidServerPort_Zero_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"]["port"] = 0;

    const auto configPath{ testDir_ + "/invalid_port_zero.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidServerPort_TooHigh_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"]["port"] = 65535;

    const auto configPath{ testDir_ + "/invalid_port_high.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidServerThreads_Zero_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"]["threads"] = 0;

    const auto configPath{ testDir_ + "/invalid_threads_zero.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidServerThreads_TooHigh_ThrowsException)
{
    auto config{ baseConfig_ };
    config["server"]["threads"] = 1025;

    const auto configPath{ testDir_ + "/invalid_threads_zero.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_SSLCertificateFileNotFound_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"]["certificate_file"] = "non_existent.crt";

    const auto configPath{ testDir_ + "/missing_ssl_cert.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_SSLPrivateKeyFileNotFound_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"]["private_key_file"] = "non_existent.key";

    const auto configPath{ testDir_ + "/missing_ssl_key.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_SSLDHParamsFileNotFound_ThrowsException)
{
    auto config{ baseConfig_ };
    config["ssl"]["dh_params_file"] = "non_existent.pem";

    const auto configPath{ testDir_ + "/missing_ssl_dh.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyDatabaseAddress_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["address"] = "";

    const auto configPath{ testDir_ + "/empty_db_address.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidDatabasePort_Zero_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["port"] = 0;

    const auto configPath{ testDir_ + "/invalid_db_port_zero.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidDatabasePort_TooHigh_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["port"] = 65535;

    const auto configPath{ testDir_ + "/invalid_db_port_high.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyDatabaseUsername_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["username"] = "";

    const auto configPath{ testDir_ + "/empty_db_username.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyDatabasePassword_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["password"] = "";

    const auto configPath{ testDir_ + "/empty_db_password.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyDatabaseDBName_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["db_name"] = "";

    const auto configPath{ testDir_ + "/empty_db_db_name.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidDatabaseMaxConnections_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["max_connections"] = 0;

    const auto configPath{ testDir_ + "/invalid_db_max_connections.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidDatabaseConnectionTimeout_ThrowsException)
{
    auto config{ baseConfig_ };
    config["database"]["connection_timeout"] = 0;

    const auto configPath{ testDir_ + "/invalid_db_connection_timeout.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
        }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyJWTSecret_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"]["secret_key"] = "";

    const auto configPath{ testDir_ + "/empty_jwt_secret.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidJWTAccessTokenExpiryMinutes_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"]["access_token_expiry_minutes"] = 0;

    const auto configPath{ testDir_ + "/invalid_jwt_access_token_expiry_minutes.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_InvalidJWTRefreshTokenExpiryDays_ThrowsException)
{
    auto config{ baseConfig_ };
    config["jwt"]["refresh_token_expiry_days"] = 0;

    const auto configPath{ testDir_ + "/invalid_jwt_refresh_token_expiry_days.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyLoggingLevel_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"]["level"] = "";

    const auto configPath{ testDir_ + "/empty_logging_level.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyLoggingAccessLog_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"]["access_log"] = "";

    const auto configPath{ testDir_ + "/empty_logging_access_log.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, Validation_EmptyLoggingErrorLog_ThrowsException)
{
    auto config{ baseConfig_ };
    config["logging"]["error_log"] = "";

    const auto configPath{ testDir_ + "/empty_logging_error_log.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, EdgeCase_MinimumValidPort)
{
    auto config{ baseConfig_ };
    config["server"]["port"] = 1;

    const auto configPath{ testDir_ + "/min_port.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerPort(), 1);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_MaximumValidPort)
{
    auto config{ baseConfig_ };
    config["server"]["port"] = 65534;

    const auto configPath{ testDir_ + "/max_port.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerPort(), 65534);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_MinimumValidThreads)
{
    auto config{ baseConfig_ };
    config["server"]["threads"] = 1;

    const auto configPath{ testDir_ + "/min_threads.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerThreads(), 1);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_MaximumValidThreads)
{
    auto config{ baseConfig_ };
    config["server"]["threads"] = 1024;

    const auto configPath{ testDir_ + "/min_threads.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerThreads(), 1024);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_MinimumValidJWTAccessExpiry)
{
    auto config{ baseConfig_ };
    config["jwt"]["access_token_expiry_minutes"] = 1;

    const auto configPath{ testDir_ + "/min_jwt_access.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getJWTAccessTokenExpiryMinutes(), 1);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_MinimumValidJWTRefreshExpiry)
{
    auto config{ baseConfig_ };
    config["jwt"]["refresh_token_expiry_days"] = 1;

    const auto configPath{ testDir_ + "/min_jwt_refresh.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getJWTRefreshTokenExpiryDays(), 1);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_LargeJWTAccessExpiry)
{
    auto config{ baseConfig_ };
	config["jwt"]["access_token_expiry_minutes"] = 525600; // 1 year in minutes

    const auto configPath{ testDir_ + "/large_jwt_access.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getJWTAccessTokenExpiryMinutes(), 525600);
    });
}

TEST_F(ConfigManagerTest, EdgeCase_LargeJWTRefreshExpiry)
{
    auto config{ baseConfig_ };
    config["jwt"]["refresh_token_expiry_days"] = 3650; // 10 years

    const auto configPath{ testDir_ + "/large_jwt_refresh.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getJWTRefreshTokenExpiryDays(), 3650);
    });
}

TEST_F(ConfigManagerTest, SpecialCase_IPv6Address)
{
    auto config{ baseConfig_ };
    config["server"]["address"] = "::1";

    const auto configPath{ testDir_ + "/ipv6_address.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerAddress(), "::1");
    });
}

TEST_F(ConfigManagerTest, SpecialCase_LocalhostAddress)
{
    auto config{ baseConfig_ };
    config["server"]["address"] = "localhost";

    const auto configPath{ testDir_ + "/localhost_address.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getServerAddress(), "localhost");
    });
}

TEST_F(ConfigManagerTest, SpecialCase_ComplexDatabasePassword)
{
    auto config{ baseConfig_ };
    config["database"]["password"] = "P@ssw0rd!123#Complex$";

    const auto configPath{ testDir_ + "/complex_password.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getDatabasePassword(), "P@ssw0rd!123#Complex$");
    });
}

TEST_F(ConfigManagerTest, SpecialCase_LongJWTSecret)
{
    auto config{ baseConfig_ };
    config["jwt"]["secret_key"] = "very_long_secret_key_that_exceeds_typical_length_requirements_1234567890";

    const auto configPath{ testDir_ + "/long_jwt_secret.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
        EXPECT_EQ(manager.getJWTSecretKey(), "very_long_secret_key_that_exceeds_typical_length_requirements_1234567890");
    });
}

TEST_F(ConfigManagerTest, TypeSafety_StringInsteadOfNumber_ReturnsDefault)
{
    auto config{ baseConfig_ };
    config["server"]["port"] = "not_a_number"; // string instead of number

    const auto configPath{ testDir_ + "/string_instead_of_number.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
        ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, TypeSafety_BoolInsteadOfString_ReturnsDefault)
{
    auto config{ baseConfig_ };
    config["server"]["address"] = true; // bool instead of string

    const auto configPath{ testDir_ + "/bool_instead_of_string.json" };
    createConfigFile(configPath, config);

    EXPECT_THROW({
		ConfigManager manager(configPath);
    }, std::runtime_error);
}

TEST_F(ConfigManagerTest, NestedConfig_AdditionalFields_Ignored)
{
    auto config{ baseConfig_ };
    // adding additional fields that are not used
    config["server"]["additional_field"] = "ignored_value";
    config["database"]["extra_config"] = { {"nested", "value"} };

    const auto configPath{ testDir_ + "/nested_config.json" };
    createConfigFile(configPath, config);

    EXPECT_NO_THROW({
        ConfigManager manager(configPath);
	    // the main fields should work fine.
	    EXPECT_EQ(manager.getServerAddress(), "0.0.0.0");
	    EXPECT_EQ(manager.getDatabaseUsername(), "chat_user");
    });
}

TEST_F(ConfigManagerTest, Integration_AllMethods_ReturnConsistentValues)
{
    const auto configPath{ testDir_ + "/integration_test.json" };
    createConfigFile(configPath, baseConfig_);

    ConfigManager manager(configPath);

    EXPECT_FALSE(manager.getServerAddress().empty());
    EXPECT_GT(manager.getServerPort(), 0u);
    EXPECT_GT(manager.getServerThreads(), 0);

    EXPECT_FALSE(manager.getSSLCertificateFile().empty());
    EXPECT_FALSE(manager.getSSLPrivateKeyFile().empty());
    EXPECT_FALSE(manager.getSSLDHParamsFile().empty());

    EXPECT_FALSE(manager.getDatabaseAddress().empty());
    EXPECT_GT(manager.getDatabasePort(), 0);
    EXPECT_FALSE(manager.getDatabaseUsername().empty());
    EXPECT_FALSE(manager.getDatabasePassword().empty());
    EXPECT_FALSE(manager.getDatabaseDBName().empty());
    EXPECT_GT(manager.getDatabaseMaxConnections(), 0u);
    EXPECT_GT(manager.getDatabaseConnectionTimeout(), 0u);

    EXPECT_FALSE(manager.getJWTSecretKey().empty());
    EXPECT_GT(manager.getJWTAccessTokenExpiryMinutes(), 0u);
    EXPECT_GT(manager.getJWTRefreshTokenExpiryDays(), 0u);

    EXPECT_FALSE(manager.getLoggingLevel().empty());
    EXPECT_FALSE(manager.getAccessLogPath().empty());
    EXPECT_FALSE(manager.getErrorLogPath().empty());
}
}

#endif // CONFIG_MANAGER_TEST_H
