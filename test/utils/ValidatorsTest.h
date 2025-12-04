#ifndef ValidatorsTests_h
#define ValidatorsTests_h

#include <gtest/gtest.h>

#include "utils/Validators.h"
#include <string>

namespace utils
{
class ValidatorsTest : public ::testing::Test
{
protected:
    const std::string EMPTY_STRING{ "" };
    const std::string VALID_LOGIN{ "user123" };
    const std::string VALID_LOGIN_WITH_UNDERSCORE{ "user_name" };
    const std::string VALID_LOGIN_MIN_LENGTH{ "usr" };
    const std::string VALID_LOGIN_MAX_LENGTH{ "a12345678901234567890123456789012345678901234567890" }; // 50 chars
    const std::string INVALID_LOGIN_TOO_SHORT{ "ab" };
    const std::string INVALID_LOGIN_TOO_LONG{ "a123456789012345678901234567890123456789012345678901" }; // 51 chars
    const std::string INVALID_LOGIN_SPECIAL_CHARS{ "user@name" };
    const std::string INVALID_LOGIN_SPACES{ "user name" };

    const std::string VALID_PASSWORD{ "password123" };
    const std::string VALID_PASSWORD_MIN_LENGTH{ "pass12" };
    const std::string VALID_PASSWORD_ONLY_LETTERS_AND_DIGITS{ "Pass1234" };
    const std::string VALID_PASSWORD_SPECIAL_CHARS{ "P@ssw0rd!" };
    const std::string INVALID_PASSWORD_TOO_SHORT{ "pass1" };
    const std::string INVALID_PASSWORD_ONLY_LETTERS{ "password" };
    const std::string INVALID_PASSWORD_ONLY_DIGITS{ "123456" };
    const std::string INVALID_PASSWORD_ONLY_SPECIAL{ "!@#$%^" };

    const std::string VALID_UUID{ "12345678-1234-1234-1234-123456789abc" };
    const std::string VALID_UUID_UPPERCASE{ "12345678-1234-1234-1234-123456789ABC" };
    const std::string VALID_UUID_NIL{ "00000000-0000-0000-0000-000000000000" };
    const std::string INVALID_UUID_SHORT{ "12345678-1234-1234-1234-123456789ab" };
    const std::string INVALID_UUID_LONG{ "12345678-1234-1234-1234-123456789abcd" };
    const std::string INVALID_UUID_NO_DASHES{ "12345678123412341234123456789abc" };
    const std::string INVALID_UUID_WRONG_FORMAT{ "12345678-1234-1234-1234_123456789abc" };
    const std::string INVALID_UUID_INVALID_CHARS{ "12345678-1234-1234-1234-123456789abx" };

    const std::string VALID_MESSAGE_SHORT{ "Hello" };
    const std::string VALID_MESSAGE_LONG{ std::string(4096, 'a') };
    const std::string INVALID_MESSAGE_EMPTY{ "" };
    const std::string INVALID_MESSAGE_TOO_LONG{ std::string(4097, 'a') };

    const std::string STRING_TO_SANITIZE{ "Hello\nWorld\t'test\"\\value\0end" };
    const std::string STRING_WITH_WHITESPACE{ "  hello world  " };

    const std::string SAFE_SQL_STRING{ "SELECT * FROM users WHERE name = 'John'" };
    const std::string SQL_INJECTION_UNION{ "admin' UNION SELECT * FROM passwords" };
    const std::string SQL_INJECTION_OR{ "' OR '1'='1" };
    const std::string SQL_INJECTION_DROP{ "test'; DROP TABLE users;" };
    const std::string SQL_INJECTION_COMMENT{ "admin'--" };
    const std::string SQL_INJECTION_MIXED_CASE{ "sElEcT * FrOm users" };

    const std::string SAFE_HTML{ "<p>Hello World</p>" };
    const std::string XSS_SCRIPT{ "<script>alert('xss')</script>" };
    const std::string XSS_JAVASCRIPT{ "javascript:alert('xss')" };
    const std::string XSS_EVENT{ "<img onerror=\"alert('xss')\" src=\"x\">" };
    const std::string XSS_MIXED_CASE{ "<ScRiPt>alert('xss')</sCrIpT>" };
    const std::string XSS_IFRAME{ "<iframe src=\"malicious.com\"></iframe>" };
};

TEST_F(ValidatorsTest, IsLoginValid_ValidLogin_ReturnsTrue)
{
    EXPECT_TRUE(Validators::isLoginValid(VALID_LOGIN));
    EXPECT_TRUE(Validators::isLoginValid(VALID_LOGIN_WITH_UNDERSCORE));
    EXPECT_TRUE(Validators::isLoginValid(VALID_LOGIN_MIN_LENGTH));
}

TEST_F(ValidatorsTest, IsLoginValid_InvalidLogin_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isLoginValid(INVALID_LOGIN_TOO_SHORT));
    EXPECT_FALSE(Validators::isLoginValid(INVALID_LOGIN_TOO_LONG));
    EXPECT_FALSE(Validators::isLoginValid(INVALID_LOGIN_SPECIAL_CHARS));
    EXPECT_FALSE(Validators::isLoginValid(INVALID_LOGIN_SPACES));
    EXPECT_FALSE(Validators::isLoginValid(EMPTY_STRING));
    EXPECT_FALSE(Validators::isLoginValid(VALID_LOGIN_MAX_LENGTH));
}

TEST_F(ValidatorsTest, IsLoginValid_EdgeCases)
{
    EXPECT_TRUE(Validators::isLoginValid("abc"));
    EXPECT_TRUE(Validators::isLoginValid(std::string(50, 'a')));
    EXPECT_FALSE(Validators::isLoginValid("ab"));
    EXPECT_FALSE(Validators::isLoginValid(std::string(51, 'a')));

    EXPECT_TRUE(Validators::isLoginValid("User123"));
    EXPECT_TRUE(Validators::isLoginValid("user_name"));
    EXPECT_TRUE(Validators::isLoginValid("TEST_USER_123"));

    EXPECT_FALSE(Validators::isLoginValid("user-name"));
    EXPECT_FALSE(Validators::isLoginValid("user.name"));
    EXPECT_FALSE(Validators::isLoginValid("user name"));
    EXPECT_FALSE(Validators::isLoginValid("user@name"));
    EXPECT_FALSE(Validators::isLoginValid("user+name"));
}

TEST_F(ValidatorsTest, IsPasswordValid_ValidPassword_ReturnsTrue)
{
    EXPECT_TRUE(Validators::isPasswordValid(VALID_PASSWORD));
    EXPECT_TRUE(Validators::isPasswordValid(VALID_PASSWORD_MIN_LENGTH));
    EXPECT_TRUE(Validators::isPasswordValid(VALID_PASSWORD_ONLY_LETTERS_AND_DIGITS));
    EXPECT_TRUE(Validators::isPasswordValid(VALID_PASSWORD_SPECIAL_CHARS));
}

TEST_F(ValidatorsTest, IsPasswordValid_InvalidPassword_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isPasswordValid(INVALID_PASSWORD_TOO_SHORT));
    EXPECT_FALSE(Validators::isPasswordValid(INVALID_PASSWORD_ONLY_LETTERS));
    EXPECT_FALSE(Validators::isPasswordValid(INVALID_PASSWORD_ONLY_DIGITS));
    EXPECT_FALSE(Validators::isPasswordValid(INVALID_PASSWORD_ONLY_SPECIAL));
    EXPECT_FALSE(Validators::isPasswordValid(EMPTY_STRING));
}

TEST_F(ValidatorsTest, IsPasswordValid_EdgeCases)
{
    EXPECT_TRUE(Validators::isPasswordValid("pass12"));
    EXPECT_FALSE(Validators::isPasswordValid("123ab"));
    EXPECT_FALSE(Validators::isPasswordValid("pass1"));

    EXPECT_TRUE(Validators::isPasswordValid("1a" + std::string(4, 'x')));
    EXPECT_TRUE(Validators::isPasswordValid("A1" + std::string(100, '!')));

    EXPECT_FALSE(Validators::isPasswordValid("password"));
    EXPECT_FALSE(Validators::isPasswordValid("PASSWORD"));
    EXPECT_FALSE(Validators::isPasswordValid("1234567890"));
    EXPECT_FALSE(Validators::isPasswordValid("!@#$%^&*"));
}

TEST_F(ValidatorsTest, IsUUIDValid_ValidUUID_ReturnsTrue)
{
    EXPECT_TRUE(Validators::isUUIDValid(VALID_UUID));
    EXPECT_TRUE(Validators::isUUIDValid(VALID_UUID_UPPERCASE));
    EXPECT_TRUE(Validators::isUUIDValid(VALID_UUID_NIL));
}

TEST_F(ValidatorsTest, IsUUIDValid_InvalidUUID_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isUUIDValid(INVALID_UUID_SHORT));
    EXPECT_FALSE(Validators::isUUIDValid(INVALID_UUID_LONG));
    EXPECT_FALSE(Validators::isUUIDValid(INVALID_UUID_NO_DASHES));
    EXPECT_FALSE(Validators::isUUIDValid(INVALID_UUID_WRONG_FORMAT));
    EXPECT_FALSE(Validators::isUUIDValid(INVALID_UUID_INVALID_CHARS));
    EXPECT_FALSE(Validators::isUUIDValid(EMPTY_STRING));
}

TEST_F(ValidatorsTest, IsUUIDValid_EdgeCases)
{
    EXPECT_TRUE(Validators::isUUIDValid("12345678-1234-1234-1234-123456789abc"));
    EXPECT_TRUE(Validators::isUUIDValid("FFFFFFFF-FFFF-FFFF-FFFF-FFFFFFFFFFFF"));
    EXPECT_TRUE(Validators::isUUIDValid("00000000-0000-0000-0000-000000000000"));

    EXPECT_FALSE(Validators::isUUIDValid("12345678-1234-1234-1234-123456789ab"));
    EXPECT_FALSE(Validators::isUUIDValid("12345678-1234-1234-1234-123456789abcd"));
    EXPECT_FALSE(Validators::isUUIDValid("1234567-8123-4123-4123-4123456789abc")); // incorrect hyphens
    EXPECT_FALSE(Validators::isUUIDValid("12345678-1234-1234_1234-123456789abc")); // wrong delimiter
}

TEST_F(ValidatorsTest, IsMessageLengthValid_ValidMessage_ReturnsTrue)
{
    EXPECT_TRUE(Validators::isMessageLengthValid(VALID_MESSAGE_SHORT));
    EXPECT_TRUE(Validators::isMessageLengthValid(VALID_MESSAGE_LONG));
    EXPECT_TRUE(Validators::isMessageLengthValid(VALID_MESSAGE_SHORT, 10));
}

TEST_F(ValidatorsTest, IsMessageLengthValid_InvalidMessage_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isMessageLengthValid(INVALID_MESSAGE_EMPTY));
    EXPECT_FALSE(Validators::isMessageLengthValid(INVALID_MESSAGE_TOO_LONG));
    EXPECT_FALSE(Validators::isMessageLengthValid(VALID_MESSAGE_LONG, 1000));
}

TEST_F(ValidatorsTest, IsMessageLengthValid_EdgeCases)
{
    EXPECT_TRUE(Validators::isMessageLengthValid("a", 1));
    EXPECT_TRUE(Validators::isMessageLengthValid(std::string(100, 'a'), 100));
    EXPECT_FALSE(Validators::isMessageLengthValid("", 100));
    EXPECT_FALSE(Validators::isMessageLengthValid(std::string(101, 'a'), 100));

    EXPECT_TRUE(Validators::isMessageLengthValid("hello", 5));
    EXPECT_FALSE(Validators::isMessageLengthValid("hello", 4));
}

TEST_F(ValidatorsTest, SanitizeString_RemovesDangerousCharacters)
{
    const auto sanitized{ Validators::sanitizeString(STRING_TO_SANITIZE) };

    EXPECT_EQ(sanitized.find('\n'), std::string::npos);
    EXPECT_EQ(sanitized.find('\r'), std::string::npos);
    EXPECT_EQ(sanitized.find('\t'), std::string::npos);
    EXPECT_EQ(sanitized.find('\0'), std::string::npos);
}

TEST_F(ValidatorsTest, SanitizeString_TrimsWhitespace)
{
    const auto sanitized{ Validators::sanitizeString(STRING_WITH_WHITESPACE) };
    EXPECT_EQ(sanitized, "hello world");
}

TEST_F(ValidatorsTest, SanitizeString_EdgeCases)
{
    EXPECT_EQ(Validators::sanitizeString(""), "");
    EXPECT_EQ(Validators::sanitizeString("   "), "");
    EXPECT_EQ(Validators::sanitizeString("\n\r\t\0"), "");

    const std::string multipleQuotes{ "'''\"\"\"\\\\\\" };
    const auto sanitized{ Validators::sanitizeString(multipleQuotes) };
    EXPECT_GT(sanitized.length(), multipleQuotes.length());
}

TEST_F(ValidatorsTest, IsSQLInjection_DetectsSQLKeywords)
{
    EXPECT_TRUE(Validators::isSQLInjection(SQL_INJECTION_UNION));
    EXPECT_TRUE(Validators::isSQLInjection(SQL_INJECTION_OR));
    EXPECT_TRUE(Validators::isSQLInjection(SQL_INJECTION_DROP));
    EXPECT_TRUE(Validators::isSQLInjection(SQL_INJECTION_MIXED_CASE));
}

TEST_F(ValidatorsTest, IsSQLInjection_SafeStrings_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isSQLInjection(VALID_LOGIN));
    EXPECT_FALSE(Validators::isSQLInjection("normal text without sql"));
    EXPECT_FALSE(Validators::isSQLInjection(EMPTY_STRING));
}

TEST_F(ValidatorsTest, IsSQLInjection_EdgeCases)
{
    EXPECT_FALSE(Validators::isSQLInjection("selection"));
    EXPECT_FALSE(Validators::isSQLInjection("oracle"));
    EXPECT_TRUE(Validators::isSQLInjection(" SELECT "));
    EXPECT_TRUE(Validators::isSQLInjection(";SELECT"));

    EXPECT_TRUE(Validators::isSQLInjection("SELECT"));
    EXPECT_TRUE(Validators::isSQLInjection("SELECT*"));
    EXPECT_TRUE(Validators::isSQLInjection("(SELECT"));
    EXPECT_FALSE(Validators::isSQLInjection("SELECTION"));
}

TEST_F(ValidatorsTest, IsXSS_DetectsXSSPatterns)
{
    EXPECT_TRUE(Validators::isXSS(XSS_SCRIPT));
    EXPECT_TRUE(Validators::isXSS(XSS_JAVASCRIPT));
    EXPECT_TRUE(Validators::isXSS(XSS_EVENT));
    EXPECT_TRUE(Validators::isXSS(XSS_MIXED_CASE));
    EXPECT_TRUE(Validators::isXSS(XSS_IFRAME));
}

TEST_F(ValidatorsTest, IsXSS_SafeStrings_ReturnsFalse)
{
    EXPECT_FALSE(Validators::isXSS(SAFE_HTML));
    EXPECT_FALSE(Validators::isXSS(VALID_MESSAGE_SHORT));
    EXPECT_FALSE(Validators::isXSS("normal text without html"));
    EXPECT_FALSE(Validators::isXSS(EMPTY_STRING));
}

TEST_F(ValidatorsTest, IsXSS_EdgeCases)
{
    EXPECT_TRUE(Validators::isXSS("<script>"));
    EXPECT_TRUE(Validators::isXSS("javascript:"));
    EXPECT_TRUE(Validators::isXSS("onload="));

    EXPECT_TRUE(Validators::isXSS("<SCRIPT>alert('xss')</SCRIPT>"));
    EXPECT_TRUE(Validators::isXSS("JavaSCript:alert('xss')"));

    EXPECT_FALSE(Validators::isXSS("scripter"));
    EXPECT_FALSE(Validators::isXSS("evaluation"));
}

TEST_F(ValidatorsTest, Integration_LoginAndPasswordValidation)
{
    EXPECT_TRUE(Validators::isLoginValid("testuser"));
    EXPECT_TRUE(Validators::isPasswordValid("test123"));

    EXPECT_FALSE(Validators::isLoginValid("test@user"));
    EXPECT_FALSE(Validators::isPasswordValid("short"));
}

TEST_F(ValidatorsTest, Integration_SanitizeAndInjectionCheck)
{
	const std::string potentiallyDangerous{ "test'; DROP TABLE users; --" };

    EXPECT_TRUE(Validators::isSQLInjection(potentiallyDangerous));

    const auto sanitized{ Validators::sanitizeString(potentiallyDangerous) };
    EXPECT_TRUE(Validators::isSQLInjection(sanitized));
}

TEST_F(ValidatorsTest, SpecialCases_SQLInjectionWithWordBoundaries)
{
    EXPECT_TRUE(Validators::isSQLInjection(" SELECT "));
    EXPECT_TRUE(Validators::isSQLInjection("(SELECT"));
    EXPECT_TRUE(Validators::isSQLInjection(";SELECT"));
    EXPECT_TRUE(Validators::isSQLInjection("SELECT*"));

    EXPECT_FALSE(Validators::isSQLInjection("SELECTION"));
    EXPECT_FALSE(Validators::isSQLInjection("RESELECT"));
}

TEST_F(ValidatorsTest, SpecialCases_XSSPartialDetection)
{
    EXPECT_TRUE(Validators::isXSS("<script"));
    EXPECT_TRUE(Validators::isXSS("javascript:"));
    EXPECT_TRUE(Validators::isXSS("onload="));

    EXPECT_TRUE(Validators::isXSS("Hello <script>alert('xss')</script> World"));
    EXPECT_TRUE(Validators::isXSS("Click here: javascript:void(0)"));
}
}

#endif // ValidatorsTests_h
