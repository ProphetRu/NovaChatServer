#include "Validators.h"
#include <regex>
#include <boost/algorithm/string.hpp>

namespace utils
{
constexpr auto MIN_LOGIN_SIZE{ 3 };
constexpr auto MAX_LOGIN_SIZE{ 50 };
constexpr auto MIN_PASSWORD_SIZE{ 6 };
constexpr auto MAX_PASSWORD_SIZE{ 128 };
constexpr auto JWT_PARTS{ 3 };

bool Validators::isLoginValid(const std::string& login) noexcept
{
    if (login.size() < MIN_LOGIN_SIZE || login.size() > MAX_LOGIN_SIZE)
    {
        return false;
    }

    const std::regex pattern{ "^[a-zA-Z0-9_]+$" };
    return std::regex_match(login, pattern);
}

bool Validators::isPasswordValid(const std::string& password) noexcept
{
    if (password.size() < MIN_PASSWORD_SIZE || password.size() > MAX_PASSWORD_SIZE) 
    {
        return false;
    }

    auto hasLetter{ false };
    auto hasDigit{ false };

    for (char c : password) 
    {
        if (std::isalpha(static_cast<unsigned char>(c))) 
        {
            hasLetter = true;
        }
        else if (std::isdigit(static_cast<unsigned char>(c))) 
        {
            hasDigit = true;
        }

        if (hasLetter && hasDigit) 
        {
            break;
        }
    }

    return hasLetter && hasDigit;
}

bool Validators::isUUIDValid(const std::string& uuid) noexcept
{
    const std::regex pattern{ R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)" };
    return std::regex_match(uuid, pattern);
}

bool Validators::isMessageLengthValid(const std::string& message, size_t maxLength) noexcept
{
    return !message.empty() && message.size() <= maxLength;
}

std::string Validators::sanitizeString(const std::string& input) noexcept
{
    auto sanitized{ input };

    std::erase(sanitized, '\0');

    std::vector<std::pair<std::string, std::string>> replacements
	{
        {"'",  "''"},
        {"\"", "\\\""},
        {"\\", "\\\\"},
        {"\0", ""},
        {"\n", " "},
        {"\r", " "},
        {"\t", " "}
    };

    for (const auto& [fst, snd] : replacements) 
    {
        boost::replace_all(sanitized, fst, snd);
    }

    boost::trim(sanitized);

    return sanitized;
}

bool Validators::isSQLInjection(const std::string& input) noexcept
{
    auto isWordBoundary = [](char c) noexcept
	{
        return !std::isalnum(static_cast<unsigned char>(c)) && c != '_';
    };

    auto upperInput{ boost::to_upper_copy(input) };

    for (const auto& keyword : sqlKeywords_) 
    {
        size_t pos{ 0 };
        while ((pos = upperInput.find(keyword, pos)) != std::string::npos) 
        {
	        const auto leftBoundary{ (pos == 0) || isWordBoundary(upperInput[pos - 1]) };

            const size_t endPos{ pos + keyword.size() };

            bool isRightBoundary{ (endPos >= upperInput.size()) || isWordBoundary(upperInput[endPos]) };

            if (leftBoundary && isRightBoundary) 
            {
                return true;
            }

            pos = endPos;
        }
    }

    return false;
}

bool Validators::isXSS(const std::string& input) noexcept
{
	const auto lowerInput{ boost::to_lower_copy(input) };

    for (const auto& pattern : xssPatterns_) 
    {
        if (lowerInput.find(pattern) != std::string::npos) 
        {
            return true;
        }
    }

    return false;
}
}
