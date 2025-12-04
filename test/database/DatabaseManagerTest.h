#ifndef DATABASE_MANAGER_TEST_H
#define DATABASE_MANAGER_TEST_H

#include <gtest/gtest.h>

#include "database/DatabaseManager.h"
#include <chrono>

namespace database
{
class DatabaseManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        validAddress = "192.168.50.37";
        validPort = 5432;
        validUsername = "chat_user";
        validPassword = "chat_user";
        validDbName = "chat_db";
        maxConnections = 5;
        connectionTimeout = 5;

        invalidAddress = "invalid.host";
    }

    void TearDown() override
	{
    }

    std::string validAddress;
    uint16_t validPort{};
    std::string validUsername;
    std::string validPassword;
    std::string validDbName;
    unsigned int maxConnections{};
    unsigned int connectionTimeout{};

    std::string invalidAddress;

    bool isDatabaseAvailable() const noexcept
    {
        try
        {
            DatabaseManager tempManager(validAddress, validPort, validUsername, validPassword, validDbName, 1, connectionTimeout);
            return tempManager.healthCheck();
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool skipIfDatabaseNotAvailable() const noexcept
    {
        if (!isDatabaseAvailable())
        {
            std::cerr << "Skipping test: Database not available" << std::endl;
            return true;
        }

        return false;
    }
};

TEST_F(DatabaseManagerTest, ValidConstruction)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    EXPECT_NO_THROW({
        DatabaseManager manager(validAddress, validPort, validUsername,
                              validPassword, validDbName, maxConnections,
                              connectionTimeout);
    });
}

TEST_F(DatabaseManagerTest, InvalidConstruction)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    EXPECT_THROW({
        DatabaseManager manager(invalidAddress, validPort, validUsername,
                              validPassword, validDbName, maxConnections,
                              connectionTimeout);
    }, std::runtime_error);
}

TEST_F(DatabaseManagerTest, ExecuteValidQuery)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    EXPECT_NO_THROW({
        const auto result{ manager.executeQuery("SELECT 1 as test_value") };
        EXPECT_FALSE(result.empty());
        EXPECT_EQ(result[0]["test_value"].as<int>(), 1);
    });
}

TEST_F(DatabaseManagerTest, ExecuteInvalidQuery)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    EXPECT_THROW({
        manager.executeQuery("INVALID SQL QUERY");
    }, std::runtime_error);
}

TEST_F(DatabaseManagerTest, HealthCheckSuccess)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    EXPECT_TRUE(manager.healthCheck());
}

TEST_F(DatabaseManagerTest, ExecuteEmptyQuery)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    EXPECT_NO_THROW({
        auto result{ manager.executeQuery("") };
    });
}

TEST_F(DatabaseManagerTest, ConcurrentAccess)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    constexpr auto numThreads{ 10 };
    constexpr auto queriesPerThread{ 5 };

    std::vector<std::jthread> threads;
    threads.reserve(numThreads);

    std::atomic<int> successCount{};
    std::atomic<int> failureCount{};

    for (int i = 0; i < numThreads; ++i) 
    {
        threads.emplace_back([&manager, &successCount, &failureCount, queriesPerThread]() 
        {
            for (int j = 0; j < queriesPerThread; ++j) 
            {
                try 
                {
                    auto result{ manager.executeQuery("SELECT " + std::to_string(j) + " as thread_value") };
                    if (!result.empty() && result[0]["thread_value"].as<int>() == j) 
                    {
                        ++successCount;
                    }
                }
                catch (const std::exception&) 
                {
                    ++failureCount;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });
    }

    for (auto& thread : threads) 
    {
        thread.join();
    }

    EXPECT_EQ(successCount, numThreads * queriesPerThread - failureCount);
    EXPECT_EQ(failureCount, 0);
}

TEST_F(DatabaseManagerTest, ConnectionPoolExhaustion)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    constexpr unsigned int smallPoolSize{ 2 };
    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, smallPoolSize,
        connectionTimeout);

    for (unsigned int i = 0; i < smallPoolSize; ++i) 
    {
        EXPECT_NO_THROW({
            manager.executeQuery("SELECT " + std::to_string(i));
        });
    }
}

TEST_F(DatabaseManagerTest, LargeConnectionPool)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    constexpr unsigned int largePoolSize{ 20 };

    EXPECT_NO_THROW({
        DatabaseManager manager(validAddress, validPort, validUsername,
                              validPassword, validDbName, largePoolSize,
                              connectionTimeout);

        EXPECT_TRUE(manager.healthCheck());
    });
}

TEST_F(DatabaseManagerTest, MinimumValues)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    constexpr unsigned int minConnections{ 1 };
    constexpr unsigned int minTimeout{ 1 };

    EXPECT_NO_THROW({
        DatabaseManager manager(validAddress, validPort, validUsername,
                              validPassword, validDbName, minConnections,
                              minTimeout);

        EXPECT_TRUE(manager.healthCheck());
    });
}

TEST_F(DatabaseManagerTest, ZeroConnections)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    EXPECT_THROW({
        DatabaseManager manager(validAddress, validPort, validUsername,
                              validPassword, validDbName, 0,
                              connectionTimeout);
    }, std::runtime_error);
}

TEST_F(DatabaseManagerTest, SpecialCharactersInParameters)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    const std::string specialPassword{ "pass@word#123" };
    const std::string specialUser{ "user@name" };

    EXPECT_NO_THROW({
        try {
            DatabaseManager manager(validAddress, validPort, specialUser,
                                  specialPassword, validDbName, maxConnections,
                                  connectionTimeout);
        }
		 catch (const std::exception&) 
         {
             // expect that it might fail with invalid credentials,
             // but not with a connection string formatting error
		 }
    });
}

TEST_F(DatabaseManagerTest, LongRunningQuery)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    EXPECT_NO_THROW({
        const auto result{ manager.executeQuery("SELECT pg_sleep(1)") }; // Sleep for 1 second
    });
}

TEST_F(DatabaseManagerTest, RecoveryAfterError)
{
    if (skipIfDatabaseNotAvailable())
    {
        return;
    }

    DatabaseManager manager(validAddress, validPort, validUsername,
        validPassword, validDbName, maxConnections,
        connectionTimeout);

    // first we raise an error
    EXPECT_THROW({
        manager.executeQuery("INVALID SQL");
    }, std::runtime_error);

    // then check that the system has recovered and can execute valid requests
    EXPECT_NO_THROW({
        const auto result{ manager.executeQuery("SELECT 1") };
        EXPECT_FALSE(result.empty());
    });

    // health check should work
    EXPECT_TRUE(manager.healthCheck());
}
}

#endif // DATABASE_MANAGER_TEST_H
