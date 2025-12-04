#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>

namespace utils
{
/**
 * @enum LogLevel
 * @brief Defines the severity levels for log messages
 *
 * The levels are ordered from least severe (Trace) to most severe (Fatal).
 * Messages are logged only if their severity level is greater than or equal
 * to the current logger level.
 */
enum class LogLevel
{
    Trace,   ///< Detailed debugging information (most verbose)
    Debug,   ///< Debugging information for developers
    Info,    ///< General operational information
    Warning, ///< Warning conditions that may require attention
    Error,   ///< Error conditions that prevent normal operation
    Fatal    ///< Severe conditions causing program termination
};

/**
 * @class Logger
 * @brief Thread-safe singleton logger for application-wide logging
 *
 * Provides configurable logging to both console and files with multiple
 * severity levels. Supports separate access logging for HTTP requests.
 * Implements the singleton pattern to ensure a single logging instance.
 *
 * @note Thread-safe for concurrent logging from multiple threads
 * @warning Must be initialized before use via initialize() method
 */
class Logger final
{
public:
    /**
     * @brief Gets the singleton instance of the Logger
     * @return Logger& Reference to the singleton Logger instance
     */
    static Logger& getInstance() noexcept;

    /**
     * @brief Deleted copy constructor
     * @note Logger should not be copied
     */
    Logger(const Logger&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note Logger should not be copied
     */
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Deleted move constructor
     * @note Logger should not be moved
     */
    Logger(Logger&&) noexcept = delete;

    /**
     * @brief Deleted move assignment operator
     * @note Logger should not be moved
     */
    Logger& operator=(Logger&&) noexcept = delete;

    /**
     * @brief Initializes the logger with configuration
     * @param level Minimum log level as string ("trace", "debug", etc.)
     * @param accessLogPath Path to access log file
     * @param errorLogPath Path to error log file
     * @param isConsoleOutput Enable/disable console output
     * @param isLogAccess Enable/disable access logging
     * @throws std::runtime_error if log files cannot be opened
     * @note Must be called before any logging methods
     */
    void initialize(const std::string& level, const std::string& accessLogPath, const std::string& errorLogPath, bool isConsoleOutput, bool isLogAccess);

    /**
     * @brief Logs a message with Trace severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void trace(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs a message with Debug severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void debug(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs a message with Info severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void info(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs a message with Warning severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void warning(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs a message with Error severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void error(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs a message with Fatal severity level
     * @param message Message text to log
     * @param component Optional component name for context
     */
    void fatal(const std::string& message, const std::string& component = "") noexcept;

    /**
     * @brief Logs an access message (HTTP request/response)
     * @param message Access log message
     */
    void access(const std::string& message) noexcept;

#ifdef UNIT_TESTING
    /**
     * @brief Resets logger state for unit testing
     * @note Only available when UNIT_TESTING is defined
     */
    void reset() noexcept
    {
        std::lock_guard configLock{ configMutex_ };
        if (accessFile_.is_open())
        {
            accessFile_.close();
        }

        if (errorFile_.is_open())
        {
            errorFile_.close();
        }

        isInitialized_ = false;
    }

    FRIEND_TEST(LoggerTest, LevelToString_AllLevels_CorrectConversion);
    FRIEND_TEST(LoggerTest, StringToLevel_ValidStrings_CorrectConversion);
    FRIEND_TEST(LoggerTest, StringToLevel_InvalidString_DefaultsToInfo);
#endif

private:
    /**
     * @brief Private default constructor
     */
    Logger() noexcept;

    /**
     * @brief Destructor that closes log files
     */
    ~Logger() noexcept;

    /**
     * @brief Internal logging implementation
     * @param level Severity level of the message
     * @param message Message text to log
     * @param component Component name for context
     */
    void log(LogLevel level, const std::string& message, const std::string& component) noexcept;

    /**
     * @brief Converts LogLevel enum to string representation
     * @param level LogLevel enum value
     * @return std::string String representation of the level
     */
    [[nodiscard]] std::string levelToString(LogLevel level) const noexcept;

    /**
     * @brief Converts string to LogLevel enum
     * @param level String representation of log level (case-insensitive)
     * @return LogLevel Corresponding LogLevel enum, defaults to Info for invalid input
     */
    [[nodiscard]] LogLevel stringToLevel(const std::string& level) const noexcept;

    /**
     * @brief Gets current timestamp for log messages
     * @return std::string Current time formatted as "DD-MM-YYYY HH:MM:SS"
     */
    [[nodiscard]] std::string getCurrentTime() const noexcept;

    /**
     * @brief Formats a complete log message with timestamp and metadata
     * @param level Severity level
     * @param message Message text
     * @param component Component name
     * @return std::string Fully formatted log message
     */
    [[nodiscard]] std::string formatMessage(LogLevel level, const std::string& message, const std::string& component) const noexcept;

private:
    std::ofstream accessFile_;  ///< File stream for access log
    std::ofstream errorFile_;   ///< File stream for error log

    LogLevel currentLevel_;     ///< Current minimum log level

    bool isConsoleOutput_;      ///< Flag for console output enablement
    bool isLogAccess_;          ///< Flag for access logging enablement

    std::string accessLogPath_; ///< Path to access log file
    std::string errorLogPath_;  ///< Path to error log file

    std::mutex accessMutex_;    ///< Mutex for access log synchronization
    std::mutex errorMutex_;     ///< Mutex for error log synchronization
    std::mutex configMutex_;    ///< Mutex for configuration synchronization

    bool isInitialized_;        ///< Flag indicating logger initialization status
};
}

/**
 * @def LOG_TRACE(message)
 * @brief Macro for logging trace-level messages with function name
 * @param message Message text to log
 */
#define LOG_TRACE(message)   utils::Logger::getInstance().trace(message, __FUNCTION__)

 /**
  * @def LOG_DEBUG(message)
  * @brief Macro for logging debug-level messages with function name
  * @param message Message text to log
  */
#define LOG_DEBUG(message) utils::Logger::getInstance().debug(message, __FUNCTION__)

  /**
   * @def LOG_INFO(message)
   * @brief Macro for logging info-level messages with function name
   * @param message Message text to log
   */
#define LOG_INFO(message) utils::Logger::getInstance().info(message, __FUNCTION__)

/**
* @def LOG_WARNING(message)
* @brief Macro for logging warning-level messages with function name
* @param message Message text to log
*/
#define LOG_WARNING(message) utils::Logger::getInstance().warning(message, __FUNCTION__)

/**
 * @def LOG_ERROR(message)
 * @brief Macro for logging error-level messages with function name
 * @param message Message text to log
 */
#define LOG_ERROR(message) utils::Logger::getInstance().error(message, __FUNCTION__)

 /**
  * @def LOG_FATAL(message)
  * @brief Macro for logging fatal-level messages with function name
  * @param message Message text to log
  */
#define LOG_FATAL(message) utils::Logger::getInstance().fatal(message, __FUNCTION__)

  /**
   * @def LOG_ACCESS(message)
   * @brief Macro for logging access messages
   * @param message Access log message
   */
#define LOG_ACCESS(message) utils::Logger::getInstance().access(message)

#endif // LOGGER_H