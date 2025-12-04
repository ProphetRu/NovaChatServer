#include "Logger.h"
#include <iostream>
#include <chrono>
#include <algorithm>

namespace utils
{
Logger& Logger::getInstance() noexcept
{
    static Logger instance;
    return instance;
}

Logger::Logger() noexcept :
    currentLevel_{ LogLevel::Info },
    isConsoleOutput_{ true },
    isLogAccess_{ true },
    isInitialized_{ false }
{
}

Logger::~Logger() noexcept
{
    if (accessFile_.is_open()) 
    {
        accessFile_.close();
    }

    if (errorFile_.is_open()) 
    {
        errorFile_.close();
    }
}

void Logger::initialize(const std::string& level, const std::string& accessLogPath, const std::string& errorLogPath, bool isConsoleOutput, bool isLogAccess)
{
    std::lock_guard configLock{ configMutex_ };

    if (isInitialized_) 
    {
        return;
    }

    currentLevel_ = stringToLevel(level);
    accessLogPath_ = accessLogPath;
    errorLogPath_ = errorLogPath;
    isConsoleOutput_ = isConsoleOutput;
    isLogAccess_ = isLogAccess;

    try 
    {
        accessFile_.open(accessLogPath_, std::ios::app);
        if (!accessFile_.is_open()) 
        {
            throw std::runtime_error{ "Cannot open access log file: " + accessLogPath_ };
        }

        errorFile_.open(errorLogPath_, std::ios::app);
        if (!errorFile_.is_open()) 
        {
            throw std::runtime_error{ "Cannot open error log file: " + errorLogPath_ };
        }

        isInitialized_ = true;

        info("Logger initialized successfully", "Logger");
        info("Access log: " + accessLogPath_, "Logger");
        info("Error log: " + errorLogPath_, "Logger");
        info("Log level: " + level, "Logger");
        info("Console output: " + std::string{ isConsoleOutput_ ? "enabled" : "disabled" }, "Logger");
        info("Log access: " + std::string{ isLogAccess_ ? "enabled" : "disabled" }, "Logger");
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Failed to initialize logger: " << e.what() << std::endl;
        throw;
    }
}

void Logger::trace(const std::string& message, const std::string& component)  noexcept
{
    log(LogLevel::Trace, message, component);
}

void Logger::debug(const std::string& message, const std::string& component)  noexcept
{
    log(LogLevel::Debug, message, component);
}

void Logger::info(const std::string& message, const std::string& component)  noexcept
{
    log(LogLevel::Info, message, component);
}

void Logger::warning(const std::string& message, const std::string& component)  noexcept
{
    log(LogLevel::Warning, message, component);
}

void Logger::error(const std::string& message, const std::string& component) noexcept
{
    log(LogLevel::Error, message, component);
}

void Logger::fatal(const std::string& message, const std::string& component)  noexcept
{
    log(LogLevel::Fatal, message, component);
}

void Logger::access(const std::string& message) noexcept
{
    if (!isInitialized_ || !isLogAccess_)
    {
        return;
    }

    std::lock_guard lock{ accessMutex_ };

    const auto formattedMessage{ "[" + getCurrentTime() + "] " + message };

    if (accessFile_.is_open()) 
    {
        accessFile_ << formattedMessage << std::endl;
        accessFile_.flush();
    }

    if (isConsoleOutput_) 
    {
        std::cout << formattedMessage << std::endl;
    }
}

void Logger::log(LogLevel level, const std::string& message, const std::string& component) noexcept
{
    auto shouldLog = [this](LogLevel level) noexcept
    {
        return level >= currentLevel_;
    };

    if (!isInitialized_ || !shouldLog(level))
    {
        return;
    }

    std::lock_guard lock{ errorMutex_ };

    const auto formattedMessage{ formatMessage(level, message, component) };

    if (errorFile_.is_open()) 
    {
        errorFile_ << formattedMessage << std::endl;
        errorFile_.flush();
    }

    if (isConsoleOutput_) 
    {
        if (level >= LogLevel::Warning) 
        {
            std::cerr << formattedMessage << std::endl;
        }
        else 
        {
            std::cout << formattedMessage << std::endl;
        }
    }
}

std::string Logger::levelToString(LogLevel level) const noexcept
{
    if (level == LogLevel::Trace)
    {
		return "Trace";
    }
    if (level == LogLevel::Debug)
    {
        return "Debug";
    }
    if (level == LogLevel::Info)
    {
        return "Info";
	}
    if (level == LogLevel::Warning)
    {
        return "Warning";
	}
    if (level == LogLevel::Error)
    {
        return "Error";
    }
    if (level == LogLevel::Fatal)
    {
        return "Fatal";
	}

	return "UNKNOWN";
}

LogLevel Logger::stringToLevel(const std::string& level) const noexcept
{
    auto levelLower{ level };
    std::ranges::transform(levelLower, levelLower.begin(), ::tolower);

    if (levelLower == "trace")
    {
        return LogLevel::Trace;
    }
    if (levelLower == "debug")
    {
        return LogLevel::Debug;
    }
    if (levelLower == "info")
    {
        return LogLevel::Info;
    }
    if (levelLower == "warning")
    {
        return LogLevel::Warning;
    }
    if (levelLower == "error")
    {
        return LogLevel::Error;
    }
    if (levelLower == "fatal")
    {
        return LogLevel::Fatal;
    }

    return LogLevel::Info;
}

std::string Logger::getCurrentTime() const noexcept
{
    using namespace std::chrono;

    const auto now{ system_clock::to_time_t(system_clock::now()) };

    std::tm tmBuf{};

#ifdef _WIN32
    localtime_s(&tmBuf, &now);
#else
    localtime_r(&now, &tmBuf);
#endif // endif _WIN32

    std::stringstream ss{};
    ss << std::put_time(&tmBuf, "%d-%m-%Y %H:%M:%S");

    return ss.str();
}

std::string Logger::formatMessage(LogLevel level, const std::string& message, const std::string& component) const noexcept
{
    std::stringstream ss{};

    ss << "[" << getCurrentTime() << "] " << "[" << levelToString(level) << "] ";

    if (!component.empty()) 
    {
        ss << "[" << component << "] ";
    }

    ss << message;
    return ss.str();
}
}
