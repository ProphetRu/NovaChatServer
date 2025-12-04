#ifndef JWT_MANAGER_TEST_H
#define JWT_MANAGER_TEST_H

#include <gtest/gtest.h>

#include "auth/JWTManager.h"
#include <chrono>

namespace auth
{
class JWTManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
	{
        secretKey_ = "very_long_secret_key_for_testing_purposes_12345";
        accessExpiryMinutes_ = 15;
        refreshExpiryDays_ = 7;

        userID_ = "user123";
        login_ = "testuser";

        jwtManager_ = std::make_unique<JWTManager>(secretKey_, accessExpiryMinutes_, refreshExpiryDays_);
    }

    void TearDown() override
	{
        jwtManager_.reset();
    }

    bool isTimeClose(const std::chrono::system_clock::time_point& t1, const std::chrono::system_clock::time_point& t2, std::chrono::seconds tolerance = std::chrono::seconds(5))
	{
        return std::abs(std::chrono::duration_cast<std::chrono::seconds>(t1 - t2).count()) <= tolerance.count();
    }

    std::string secretKey_;
    unsigned int accessExpiryMinutes_{};
    unsigned int refreshExpiryDays_{};
    std::string userID_;
    std::string login_;
    std::unique_ptr<JWTManager> jwtManager_;
};

TEST_F(JWTManagerTest, Constructor_ValidParameters_Success)
{
    EXPECT_NO_THROW({ JWTManager manager(secretKey_, accessExpiryMinutes_, refreshExpiryDays_); });
}

TEST_F(JWTManagerTest, Constructor_EmptySecretKey_ThrowsException)
{
    EXPECT_THROW({
        JWTManager manager("", accessExpiryMinutes_, refreshExpiryDays_);
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, Constructor_ShortSecretKey_WarningLogged)
{
    // this test checks that the short key does not cause an exception, only a warning.
    EXPECT_NO_THROW({ JWTManager manager("short", 15, 7); });
}

TEST_F(JWTManagerTest, GenerateAccessToken_ValidParameters_ReturnsToken)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 50);
}

TEST_F(JWTManagerTest, GenerateAccessToken_EmptyUserId_ThrowsException)
{
    EXPECT_THROW({
        const auto token{ jwtManager_->generateAccessToken("", login_) };
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, GenerateAccessToken_EmptyLogin_ThrowsException)
{
    EXPECT_THROW({
        const auto token{ jwtManager_->generateAccessToken(userID_, "") };
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, GenerateAccessToken_DifferentUsers_DifferentTokens)
{
    const auto token1{ jwtManager_->generateAccessToken("user1", "login1") };
    const auto token2{ jwtManager_->generateAccessToken("user2", "login2") };

    EXPECT_NE(token1, token2);
}

TEST_F(JWTManagerTest, GenerateAccessToken_SameUserSameLogin_SameTokenStructure)
{
    const auto token1{ jwtManager_->generateAccessToken(userID_, login_) };
    const auto token2{ jwtManager_->generateAccessToken(userID_, login_) };

    EXPECT_EQ(token1, token2);
}

TEST_F(JWTManagerTest, GenerateRefreshToken_ValidParameters_ReturnsToken)
{
    const auto token{ jwtManager_->generateRefreshToken(userID_) };

    EXPECT_FALSE(token.empty());
    EXPECT_GT(token.length(), 50);
}

TEST_F(JWTManagerTest, GenerateRefreshToken_EmptyUserId_ThrowsException)
{
    EXPECT_THROW({
        const auto toekn{ jwtManager_->generateRefreshToken("") };
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, GenerateRefreshToken_DifferentUsers_DifferentTokens)
{
    const auto token1{ jwtManager_->generateRefreshToken("user1") };
    const auto token2{ jwtManager_->generateRefreshToken("user2") };

    EXPECT_NE(token1, token2);
}

TEST_F(JWTManagerTest, VerifyAndDecode_ValidAccessToken_ReturnsValidPayload)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_TRUE(payload.isValid);
    EXPECT_EQ(payload.userID, userID_);
    EXPECT_EQ(payload.login, login_);
    EXPECT_EQ(payload.type, "access");
    EXPECT_TRUE(payload.isAccessToken());
    EXPECT_FALSE(payload.isRefreshToken());
    EXPECT_GT(payload.expiresAt, std::chrono::system_clock::now());
}

TEST_F(JWTManagerTest, VerifyAndDecode_ValidRefreshToken_ReturnsValidPayload)
{
    const auto token{ jwtManager_->generateRefreshToken(userID_) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_TRUE(payload.isValid);
    EXPECT_EQ(payload.userID, userID_);
    EXPECT_EQ(payload.type, "refresh");
    EXPECT_TRUE(payload.isRefreshToken());
    EXPECT_FALSE(payload.isAccessToken());
    EXPECT_GT(payload.expiresAt, std::chrono::system_clock::now());
}

TEST_F(JWTManagerTest, VerifyAndDecode_EmptyToken_ThrowsException)
{
    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode("") };
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, VerifyAndDecode_InvalidToken_ThrowsException)
{
    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode("invalid.token.here") };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, VerifyAndDecode_TokenWithWrongSecret_ThrowsException)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    // create a manager with a different secret key
    JWTManager other_manager("different_secret_key_12345678901234567890", 15, 7);

    EXPECT_THROW({
        const auto payload{ other_manager.verifyAndDecode(token) };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, VerifyAndDecode_ModifiedToken_ThrowsException)
{
    auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    // modifying the token (changing one character)
    token[token.length() - 5] = (token[token.length() - 5] == 'a') ? 'b' : 'a';

    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode(token) };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, TokenBlacklist_AddAndCheck_WorksCorrectly)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    EXPECT_FALSE(jwtManager_->isTokenBlacklisted(token));

    jwtManager_->addTokenToBlacklist(token);

    EXPECT_TRUE(jwtManager_->isTokenBlacklisted(token));
}

TEST_F(JWTManagerTest, TokenBlacklist_EmptyToken_Ignored)
{
    EXPECT_NO_THROW({
        jwtManager_->addTokenToBlacklist("");
    });

    EXPECT_FALSE(jwtManager_->isTokenBlacklisted(""));
}

TEST_F(JWTManagerTest, TokenBlacklist_BlacklistedToken_ThrowsOnVerify)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    jwtManager_->addTokenToBlacklist(token);

    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode(token) };
    }, std::invalid_argument);
}

TEST_F(JWTManagerTest, TokenBlacklist_MultipleTokens_Independent)
{
    const auto token1{ jwtManager_->generateAccessToken("user1", "login1") };
    const auto token2{ jwtManager_->generateAccessToken("user2", "login2") };

    jwtManager_->addTokenToBlacklist(token1);

    EXPECT_TRUE(jwtManager_->isTokenBlacklisted(token1));
    EXPECT_FALSE(jwtManager_->isTokenBlacklisted(token2));
}

TEST_F(JWTManagerTest, GetTokenExpiry_ValidToken_ReturnsExpiry)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };
    const auto expiry{ jwtManager_->getTokenExpiry(token) };

    EXPECT_GT(expiry, std::chrono::system_clock::now());

    const auto expected_expiry{ std::chrono::system_clock::now() + std::chrono::minutes(accessExpiryMinutes_) };
    EXPECT_TRUE(isTimeClose(expiry, expected_expiry, std::chrono::seconds(10)));
}

TEST_F(JWTManagerTest, GetTokenExpiry_InvalidToken_ThrowsException)
{
    EXPECT_THROW({
        const auto exp{ jwtManager_->getTokenExpiry("invalid.token.here") };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, GetTokenExpiry_EmptyToken_ThrowsException)
{
    EXPECT_THROW({
        const auto exp{ jwtManager_->getTokenExpiry("") };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, EdgeCase_VeryShortTokenLifetime)
{
    JWTManager short_life_manager(secretKey_, 1, 1);

    const auto token{ short_life_manager.generateAccessToken(userID_, login_) };
    const auto payload{ short_life_manager.verifyAndDecode(token) };

    EXPECT_TRUE(payload.isValid);

    // check that the expiration time is in the future (but very close)
    EXPECT_GT(payload.expiresAt, std::chrono::system_clock::now());
}

TEST_F(JWTManagerTest, EdgeCase_VeryLongTokenLifetime)
{
    JWTManager long_life_manager(secretKey_, 525600, 3650); // 1 year access, 10 years refresh

    const auto accessToken{ long_life_manager.generateAccessToken(userID_, login_) };
    const auto refreshToken{ long_life_manager.generateRefreshToken(userID_) };

    const auto accessPayload{ long_life_manager.verifyAndDecode(accessToken) };
    const auto refreshPayload{ long_life_manager.verifyAndDecode(refreshToken) };

    EXPECT_TRUE(accessPayload.isValid);
    EXPECT_TRUE(refreshPayload.isValid);

    // check that the expiration time is far in the future
    EXPECT_GT(accessPayload.expiresAt, std::chrono::system_clock::now() + std::chrono::hours(23));
    EXPECT_GT(refreshPayload.expiresAt, std::chrono::system_clock::now() + std::chrono::days(3649));
}

TEST_F(JWTManagerTest, EdgeCase_SpecialCharactersInUserIdAndLogin)
{
    const std::string specialUserID{ "user@123#special$" };
    const std::string specialLogin{ "login-with-special-chars_123" };

    const auto token{ jwtManager_->generateAccessToken(specialUserID, specialLogin) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_TRUE(payload.isValid);
    EXPECT_EQ(payload.userID, specialUserID);
    EXPECT_EQ(payload.login, specialLogin);
}

TEST_F(JWTManagerTest, EdgeCase_VeryLongUserIdAndLogin)
{
    const std::string longUserID(1000, 'x');
    const std::string longLogin(1000, 'y');

    const auto token{ jwtManager_->generateAccessToken(longUserID, longLogin) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_TRUE(payload.isValid);
    EXPECT_EQ(payload.userID, longUserID);
    EXPECT_EQ(payload.login, longLogin);
}

TEST_F(JWTManagerTest, TokenType_AccessToken_HasCorrectType)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_EQ(payload.type, "access");
    EXPECT_TRUE(payload.isAccessToken());
    EXPECT_FALSE(payload.isRefreshToken());
}

TEST_F(JWTManagerTest, TokenType_RefreshToken_HasCorrectType)
{
    const auto token{ jwtManager_->generateRefreshToken(userID_) };
    const auto payload{ jwtManager_->verifyAndDecode(token) };

    EXPECT_EQ(payload.type, "refresh");
    EXPECT_TRUE(payload.isRefreshToken());
    EXPECT_FALSE(payload.isAccessToken());
}

TEST_F(JWTManagerTest, TokenReuse_SameToken_ConsistentVerification)
{
    const auto token{ jwtManager_->generateAccessToken(userID_, login_) };

    for (int i = 0; i < 10; ++i) 
    {
        const auto payload{ jwtManager_->verifyAndDecode(token) };
        EXPECT_TRUE(payload.isValid);
        EXPECT_EQ(payload.userID, userID_);
        EXPECT_EQ(payload.login, login_);
    }
}

TEST_F(JWTManagerTest, Security_TokenWithoutRequiredClaims_FailsVerification)
{
    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode("eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyfQ.SflKxwRJSMeKKF2QT4fwpMeJf36POk6yJV_adQssw5c") };
    }, std::runtime_error);
}

TEST_F(JWTManagerTest, Integration_FullTokenLifecycle)
{
    // 1. generating an access token
    const auto accessToken{ jwtManager_->generateAccessToken(userID_, login_) };
    EXPECT_FALSE(accessToken.empty());

    // 2. access token verification
    const auto accessPayload{ jwtManager_->verifyAndDecode(accessToken) };
    EXPECT_TRUE(accessPayload.isValid);
    EXPECT_EQ(accessPayload.userID, userID_);
    EXPECT_EQ(accessPayload.login, login_);
    EXPECT_TRUE(accessPayload.isAccessToken());

    // 3. generating a refresh token
    const auto refreshToken{ jwtManager_->generateRefreshToken(userID_) };
    EXPECT_FALSE(refreshToken.empty());

    // 4. verification of refresh token
    const auto refreshPayload{ jwtManager_->verifyAndDecode(refreshToken) };
    EXPECT_TRUE(refreshPayload.isValid);
    EXPECT_EQ(refreshPayload.userID, userID_);
    EXPECT_TRUE(refreshPayload.isRefreshToken());

    // 5. adding an access token to the blacklist
    jwtManager_->addTokenToBlacklist(accessToken);
    EXPECT_TRUE(jwtManager_->isTokenBlacklisted(accessToken));

    // 6. attempting to verify a blocked token
    EXPECT_THROW({
        const auto payload{ jwtManager_->verifyAndDecode(accessToken) };
    }, std::invalid_argument);

    // 7. the refresh token should still work.
    const auto refreshPayloadAfter{ jwtManager_->verifyAndDecode(refreshToken) };
    EXPECT_TRUE(refreshPayloadAfter.isValid);
}
}

#endif // JWT_MANAGER_TEST_H
