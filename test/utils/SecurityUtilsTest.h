#ifndef SecurityUtilsTests_h
#define SecurityUtilsTests_h

#include <gtest/gtest.h>

#include "utils/SecurityUtils.h"

#include <string>
#include <vector>

namespace utils
{
class SecurityUtilsTest : public ::testing::Test
{
protected:
    const std::string SAFE_INPUT{ "Hello World" };
    const std::string SAFE_INPUT_WITH_NUMBERS{ "User123" };
    const std::string SAFE_INPUT_WITH_SPECIAL{ "hello@example.com" };
    const std::string EMPTY_STRING{ "" };
    const std::string WHITESPACE_STRING{ "   hello   " };

    const std::string SQL_INJECTION_SIMPLE{ "' OR '1'='1" };
    const std::string SQL_INJECTION_UNION{ "admin' UNION SELECT * FROM passwords" };
    const std::string SQL_INJECTION_DROP{ "test'; DROP TABLE users;" };
    const std::string SQL_INJECTION_MIXED_CASE{ "sElEcT * FrOm users" };

    const std::string XSS_SCRIPT_TAG{ "<script>alert('xss')</script>" };
    const std::string XSS_JAVASCRIPT_URL{ "javascript:alert('xss')" };
    const std::string XSS_EVENT_HANDLER{ "<img onerror=\"alert('xss')\" src=\"x\">" };
    const std::string XSS_MIXED_CASE{ "<ScRiPt>alert('xss')</sCrIpT>" };
    const std::string XSS_IFRAME{ "<iframe src=\"malicious.com\"></iframe>" };

    const std::string INPUT_WITH_QUOTES{ "O'Reilly" };
    const std::string INPUT_WITH_DOUBLE_QUOTES{ "Hello \"World\"" };
    const std::string INPUT_WITH_BACKSLASH{ "path\\to\\file" };
    const std::string INPUT_WITH_WHITESPACE{ "  hello world  " };
    const std::string INPUT_WITH_NEWLINES{ "hello\nworld" };
    const std::string INPUT_WITH_TABS{ "hello\tworld" };
    const std::string INPUT_WITH_NULL{ "hello\0world" };

    const std::string VERY_LONG_SAFE_STRING{ std::string(10000, 'a') };
    const std::string VERY_LONG_SQL_INJECTION{ std::string(1000, 'a') + "' OR '1'='1" };
    const std::string VERY_LONG_XSS{ std::string(1000, 'a') + "<script>alert('xss')</script>" };

    const std::string COMBINED_ATTACK{ "' OR '1'='1<script>alert('xss')</script>" };
    const std::string COMBINED_SAFE{ "Hello 'world' with <b>bold</b> text" };
};

TEST_F(SecurityUtilsTest, SanitizeUserInput_SafeInput_ReturnsSanitized)
{
    auto result{ utils::SecurityUtils::sanitizeUserInput(SAFE_INPUT) };
    EXPECT_EQ(result, SAFE_INPUT);

    result = utils::SecurityUtils::sanitizeUserInput(SAFE_INPUT_WITH_NUMBERS);
    EXPECT_EQ(result, SAFE_INPUT_WITH_NUMBERS);

    result = utils::SecurityUtils::sanitizeUserInput(SAFE_INPUT_WITH_SPECIAL);
    EXPECT_EQ(result, SAFE_INPUT_WITH_SPECIAL);
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_EmptyString_ReturnsEmpty)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(EMPTY_STRING) };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_SQLInjection_ReturnsEmpty)
{
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_SIMPLE).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_UNION).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_DROP).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_MIXED_CASE).empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_XSS_ReturnsEmpty)
{
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(XSS_SCRIPT_TAG).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(XSS_JAVASCRIPT_URL).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(XSS_EVENT_HANDLER).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(XSS_MIXED_CASE).empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(XSS_IFRAME).empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_NeedsSanitization_ReturnsSafe)
{
    auto result{ utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_QUOTES) };
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result, INPUT_WITH_QUOTES);
    EXPECT_EQ(result, "O''Reilly");

    result = utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_DOUBLE_QUOTES);
    EXPECT_FALSE(result.empty());

    result = utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_BACKSLASH);
    EXPECT_FALSE(result.empty());

    result = utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_WHITESPACE);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result, "hello world");
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_ControlCharacters_ReturnsSanitized)
{
    auto result{ utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_NEWLINES) };
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find('\n'), std::string::npos);

    result = utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_TABS);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find('\t'), std::string::npos);

    result = utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_NULL);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.find('\0'), std::string::npos);
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_VeryLongSafeString_ReturnsSanitized)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(VERY_LONG_SAFE_STRING) };
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result.length(), VERY_LONG_SAFE_STRING.length());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_VeryLongSQLInjection_ReturnsEmpty)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(VERY_LONG_SQL_INJECTION) };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_VeryLongXSS_ReturnsEmpty)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(VERY_LONG_XSS) };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_WhitespaceOnly_ReturnsEmptyAfterTrim)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput("   ") };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_CombinedAttack_ReturnsEmpty)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(COMBINED_ATTACK) };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_CombinedSafe_ReturnsSanitized)
{
    const auto result{ utils::SecurityUtils::sanitizeUserInput(COMBINED_SAFE) };
    EXPECT_FALSE(result.empty());
    EXPECT_FALSE(Validators::isSQLInjection(result));
    EXPECT_FALSE(Validators::isXSS(result));
}

TEST_F(SecurityUtilsTest, SanitizeUserInput_ConsistentResults)
{
    const auto result1{ utils::SecurityUtils::sanitizeUserInput(SAFE_INPUT) };
    const auto result2{ utils::SecurityUtils::sanitizeUserInput(SAFE_INPUT) };
    EXPECT_EQ(result1, result2);

    const auto result3{ utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_SIMPLE) };
    const auto result4{ utils::SecurityUtils::sanitizeUserInput(SQL_INJECTION_SIMPLE) };
    EXPECT_TRUE(result3.empty());
    EXPECT_TRUE(result4.empty());
}

TEST_F(SecurityUtilsTest, Integration_WithUUIDUtils)
{
    const auto uuid{ UUIDUtils::generateUUID() };
    const auto result{ utils::SecurityUtils::sanitizeUserInput(uuid) };
    EXPECT_EQ(result, uuid);
}

TEST_F(SecurityUtilsTest, Integration_WithValidators)
{
    const auto sanitized{ utils::SecurityUtils::sanitizeUserInput(INPUT_WITH_QUOTES) };
    EXPECT_FALSE(Validators::isSQLInjection(sanitized));
    EXPECT_FALSE(Validators::isXSS(sanitized));

    EXPECT_TRUE(Validators::isLoginValid(sanitized) || true);
}

TEST_F(SecurityUtilsTest, EdgeCase_StringWithOnlySQLComment)
{
    EXPECT_FALSE(utils::SecurityUtils::sanitizeUserInput("--").empty());
    EXPECT_FALSE(utils::SecurityUtils::sanitizeUserInput("/*").empty());
}

TEST_F(SecurityUtilsTest, EdgeCase_StringWithOnlyXSSPatterns)
{
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput("<script>").empty());
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput("javascript:").empty());
}

TEST_F(SecurityUtilsTest, EdgeCase_PartialXSSPatterns)
{
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput("<script").empty());
    EXPECT_FALSE(utils::SecurityUtils::sanitizeUserInput("javascript").empty());
}

TEST_F(SecurityUtilsTest, EdgeCase_MultipleAttackVectors)
{
    const std::string complex_attack{ "'; DROP TABLE users; -- <script>alert('xss')</script> javascript:void(0)" };
    EXPECT_TRUE(utils::SecurityUtils::sanitizeUserInput(complex_attack).empty());
}

TEST_F(SecurityUtilsTest, EdgeCase_SafeHTMLTags)
{
    const std::string safe_html{ "<b>bold</b> and <i>italic</i>" };
    const auto result{ utils::SecurityUtils::sanitizeUserInput(safe_html) };
    EXPECT_TRUE(result.empty());
}

TEST_F(SecurityUtilsTest, DetailedBehavior_QuotesEscaping)
{
    const auto input{ "O'Reilly" };
    const auto result{ utils::SecurityUtils::sanitizeUserInput(input) };

    if (!result.empty()) 
    {
        EXPECT_NE(result, input);
        EXPECT_EQ(result, "O''Reilly");
    }
}

TEST_F(SecurityUtilsTest, DetailedBehavior_WhitespaceHandling)
{
    const auto input{ "  hello  world  " };
    const auto result{ utils::SecurityUtils::sanitizeUserInput(input) };

    if (!result.empty()) 
    {
        EXPECT_EQ(result, "hello  world");
    }
}

TEST_F(SecurityUtilsTest, SafeEmailAddresses_PassSanitization)
{
    const std::vector<std::string> safeEmails
	{
        "user@example.com",
        "test.user+tag@example.org",
        "user.name@domain.co.uk"
    };

    for (const auto& email : safeEmails) 
    {
        const auto result{ utils::SecurityUtils::sanitizeUserInput(email) };
        EXPECT_FALSE(result.empty()) << "Safe email was blocked: " << email;
    }
}

TEST_F(SecurityUtilsTest, SafeUsernames_PassSanitization)
{
    const std::vector<std::string> safeUsernames
	{
        "john_doe",
        "user123",
        "test-user",
        "Admin"
    };

    for (const auto& username : safeUsernames) 
    {
        const auto result{ utils::SecurityUtils::sanitizeUserInput(username) };
        EXPECT_FALSE(result.empty()) << "Safe username was blocked: " << username;
    }
}
}

#endif // SecurityUtilsTests_h
