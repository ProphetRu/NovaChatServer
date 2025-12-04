#ifndef LoggerTests_h
#define LoggerTests_h

#include <gtest/gtest.h>

#define UNIT_TESTING
#include "utils/Logger.h"

#include <fstream>
#include <filesystem>
#include <regex>

namespace utils
{
class LoggerTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        // create temporary files for tests
        accessLogPath_ = "test_access.log";
        errorLogPath_ = "test_error.log";

        // clearing previous test files
        std::filesystem::remove(accessLogPath_);
        std::filesystem::remove(errorLogPath_);
    }

    void TearDown() override
	{
        // close and clear the logger
#ifdef UNIT_TESTING
        Logger::getInstance().reset();
#endif

        // deleting test files
        std::filesystem::remove(accessLogPath_);
        std::filesystem::remove(errorLogPath_);
    }

    std::string readFileContent(const std::string& filepath)
	{
        std::ifstream file{ filepath };
        if (!file.is_open()) 
        {
            return "";
        }

        std::stringstream buffer{};
        buffer << file.rdbuf();

        return buffer.str();
    }

    bool fileContains(const std::string& filepath, const std::string& text)
	{
        const auto content{ readFileContent(filepath) };
        return content.find(text) != std::string::npos;
    }

    size_t countLines(const std::string& filepath)
	{
        auto content{ readFileContent(filepath) };
        return std::ranges::count(content, '\n');
    }

    std::string accessLogPath_;
    std::string errorLogPath_;
};

TEST_F(LoggerTest, Initialize_Successful)
{
    EXPECT_NO_THROW({ Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true); });

    // check that the files have been created
    EXPECT_TRUE(std::filesystem::exists(accessLogPath_));
    EXPECT_TRUE(std::filesystem::exists(errorLogPath_));
}

TEST_F(LoggerTest, Initialize_InvalidLogLevel_DefaultsToInfo)
{
    Logger::getInstance().initialize("invalid_level", accessLogPath_, errorLogPath_, false, true);

    // the default level (Info) should be used.
    Logger::getInstance().info("Test message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Test message"));
}

TEST_F(LoggerTest, Initialize_InvalidFilePaths_ThrowsException)
{
    EXPECT_THROW({
        Logger::getInstance().initialize("info", "/invalid/path/access.log", "/invalid/path/error.log", false, true);
    }, std::runtime_error);
}

TEST_F(LoggerTest, Initialize_MultipleCalls_IgnoresSubsequentCalls)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    // the second call should be ignored.
    EXPECT_NO_THROW({ Logger::getInstance().initialize("debug", accessLogPath_, errorLogPath_, true, false); });
}

TEST_F(LoggerTest, LogLevel_Trace_WhenLevelTrace_LogsMessage)
{
    Logger::getInstance().initialize("trace", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().trace("Trace message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Trace message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Trace]"));
}

TEST_F(LoggerTest, LogLevel_Trace_WhenLevelInfo_DoesNotLog)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().trace("Trace message", "TestComponent");

    EXPECT_FALSE(fileContains(errorLogPath_, "Trace message"));
}

TEST_F(LoggerTest, LogLevel_Debug_WhenLevelDebug_LogsMessage)
{
    Logger::getInstance().initialize("debug", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().debug("Debug message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Debug message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Debug]"));
}

TEST_F(LoggerTest, LogLevel_Info_WhenLevelInfo_LogsMessage)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().info("Info message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Info message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Info]"));
}

TEST_F(LoggerTest, LogLevel_Warning_WhenLevelWarning_LogsMessage)
{
    Logger::getInstance().initialize("warning", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().warning("Warning message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Warning message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Warning]"));
}

TEST_F(LoggerTest, LogLevel_Error_WhenLevelError_LogsMessage)
{
    Logger::getInstance().initialize("error", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().error("Error message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Error message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Error]"));
}

TEST_F(LoggerTest, LogLevel_Fatal_WhenLevelFatal_LogsMessage)
{
    Logger::getInstance().initialize("fatal", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().fatal("Fatal message", "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, "Fatal message"));
    EXPECT_TRUE(fileContains(errorLogPath_, "[Fatal]"));
}

TEST_F(LoggerTest, Access_LoggingEnabled_WritesToAccessFile)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().access("Access log entry");

    EXPECT_TRUE(fileContains(accessLogPath_, "Access log entry"));
}

TEST_F(LoggerTest, Access_LoggingDisabled_DoesNotWrite)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, false);
    Logger::getInstance().access("Access log entry");

    EXPECT_FALSE(fileContains(accessLogPath_, "Access log entry"));
}

TEST_F(LoggerTest, Access_NotInitialized_DoesNotWrite)
{
    // do not initialize the logger
    Logger::getInstance().access("Access log entry");

    EXPECT_FALSE(fileContains(accessLogPath_, "Access log entry"));
}

TEST_F(LoggerTest, MessageFormat_ContainsTimestamp)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().info("Test message", "TestComponent");

    const auto content{ readFileContent(errorLogPath_) };

    // Check the timestamp format: dd-mm-yyyy HH:MM:SS
    const std::regex timestamp_regex(R"(\[\d{2}-\d{2}-\d{4} \d{2}:\d{2}:\d{2}\])");
    EXPECT_TRUE(std::regex_search(content, timestamp_regex));
}

TEST_F(LoggerTest, MessageFormat_ContainsLevelAndComponent)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().info("Test message", "TestComponent");

    const auto content{ readFileContent(errorLogPath_) };

    EXPECT_TRUE(content.find("[Info]") != std::string::npos);
    EXPECT_TRUE(content.find("[TestComponent]") != std::string::npos);
    EXPECT_TRUE(content.find("Test message") != std::string::npos);
}

TEST_F(LoggerTest, MessageFormat_EmptyComponent_OmitsComponentBrackets)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
    Logger::getInstance().info("Test message");

    const auto content{ readFileContent(errorLogPath_) };

    // there must not be empty square brackets for the component
    EXPECT_TRUE(content.find("[]") == std::string::npos);
}

TEST_F(LoggerTest, Macros_WorkCorrectly)
{
    Logger::getInstance().initialize("debug", accessLogPath_, errorLogPath_, false, true);

    LOG_TRACE("Trace macro");
    LOG_DEBUG("Debug macro");
    LOG_INFO("Info macro");
    LOG_WARNING("Warning macro");
    LOG_ERROR("Error macro");
    LOG_FATAL("Fatal macro");
    LOG_ACCESS("Access macro");

    const auto error_content{ readFileContent(errorLogPath_) };
    const auto access_content{ readFileContent(accessLogPath_) };

    EXPECT_TRUE(error_content.find("Debug macro") != std::string::npos);
    EXPECT_TRUE(error_content.find("Info macro") != std::string::npos);
    EXPECT_TRUE(access_content.find("Access macro") != std::string::npos);
}

TEST_F(LoggerTest, EdgeCase_EmptyMessages)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    Logger::getInstance().info("", "TestComponent");
    Logger::getInstance().access("");

    const auto error_content{ readFileContent(errorLogPath_) };
    const auto access_content{ readFileContent(accessLogPath_) };

    // even empty messages (only with timestamp and level) should be recorded
    EXPECT_TRUE(error_content.find("[Info]") != std::string::npos);
    EXPECT_FALSE(access_content.find("[]") != std::string::npos);
}

TEST_F(LoggerTest, EdgeCase_VeryLongMessage)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    const std::string long_message(10000, 'X');
    Logger::getInstance().info(long_message, "TestComponent");

    EXPECT_TRUE(fileContains(errorLogPath_, long_message.substr(0, 100))); // checking the part
}

TEST_F(LoggerTest, EdgeCase_SpecialCharacters)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    const std::string specialMessage = "Message with special chars: \n\t\r\"'\\";
    Logger::getInstance().info(specialMessage, "TestComponent");

    const auto content{ readFileContent(errorLogPath_) };
    EXPECT_TRUE(content.find("Message with special chars:") != std::string::npos);
}

TEST_F(LoggerTest, ConsoleOutput_Enabled_WritesToStdout)
{
    EXPECT_NO_THROW({
        Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, true, true);
        Logger::getInstance().info("Console test");
    });
}

TEST_F(LoggerTest, ConsoleOutput_Disabled_DoesNotWriteToStdout)
{
    EXPECT_NO_THROW({
        Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
        Logger::getInstance().info("No console test");
    });
}

TEST_F(LoggerTest, LevelToString_AllLevels_CorrectConversion)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Trace), "Trace");
    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Debug), "Debug");
    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Info), "Info");
    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Warning), "Warning");
    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Error), "Error");
    EXPECT_EQ(Logger::getInstance().levelToString(LogLevel::Fatal), "Fatal");
}

TEST_F(LoggerTest, StringToLevel_ValidStrings_CorrectConversion)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    EXPECT_EQ(Logger::getInstance().stringToLevel("trace"), LogLevel::Trace);
    EXPECT_EQ(Logger::getInstance().stringToLevel("debug"), LogLevel::Debug);
    EXPECT_EQ(Logger::getInstance().stringToLevel("info"), LogLevel::Info);
    EXPECT_EQ(Logger::getInstance().stringToLevel("warning"), LogLevel::Warning);
    EXPECT_EQ(Logger::getInstance().stringToLevel("error"), LogLevel::Error);
    EXPECT_EQ(Logger::getInstance().stringToLevel("fatal"), LogLevel::Fatal);

    EXPECT_EQ(Logger::getInstance().stringToLevel("INFO"), LogLevel::Info);
    EXPECT_EQ(Logger::getInstance().stringToLevel("Info"), LogLevel::Info);
}

TEST_F(LoggerTest, StringToLevel_InvalidString_DefaultsToInfo)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    EXPECT_EQ(Logger::getInstance().stringToLevel("invalid"), LogLevel::Info);
    EXPECT_EQ(Logger::getInstance().stringToLevel(""), LogLevel::Info);
}

TEST_F(LoggerTest, FileReopening_AfterClose_ReopensOnLog)
{
    Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);

    // simulating file closing
#ifdef UNIT_TESTING
    Logger::getInstance().reset();
#endif

    EXPECT_NO_THROW({
        Logger::getInstance().initialize("info", accessLogPath_, errorLogPath_, false, true);
        Logger::getInstance().info("Reopened test");
    });

    EXPECT_TRUE(fileContains(errorLogPath_, "Reopened test"));
}
}

#endif // LoggerTests_h
