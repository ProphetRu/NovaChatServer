#ifndef PasswordHasherTests_h
#define PasswordHasherTests_h

#include <gtest/gtest.h>

#include "utils/PasswordHasher.h"

#include <string>
#include <vector>
#include <algorithm>

namespace utils
{
class PasswordHasherTest : public ::testing::Test
{
protected:
    const std::string EMPTY_STRING{ "" };
    const std::string TEST_PASSWORD{ "mySecurePassword123" };
    const std::string TEST_SALT{ "randomSaltValue" };
    const std::string SPECIAL_CHARS_PASSWORD{ "p@$$w0rd!<>#%&" };
    const std::string LONG_PASSWORD{ std::string(1000, 'a') }; // long password

    const std::string MD5_EMPTY{ "d41d8cd98f00b204e9800998ecf8427e" };
    const std::string MD5_ABC{ "900150983cd24fb0d6963f7d28e17f72" };
    const std::string SHA256_EMPTY{ "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" };
    const std::string SHA256_ABC{ "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad" };
};

TEST_F(PasswordHasherTest, MD5_ValidInput_ReturnsCorrectHash)
{
    EXPECT_EQ(utils::PasswordHasher::md5("abc"), MD5_ABC);
    EXPECT_EQ(utils::PasswordHasher::md5(""), MD5_EMPTY);
}

TEST_F(PasswordHasherTest, MD5_SameInput_ReturnsSameHash)
{
	const auto hash1{ utils::PasswordHasher::md5(TEST_PASSWORD) };
    const auto hash2{ utils::PasswordHasher::md5(TEST_PASSWORD) };
    EXPECT_EQ(hash1, hash2);
    EXPECT_FALSE(hash1.empty());
}

TEST_F(PasswordHasherTest, MD5_DifferentInput_ReturnsDifferentHash)
{
	const auto hash1{ utils::PasswordHasher::md5("password1") };
    const auto hash2{ utils::PasswordHasher::md5("password2") };
    EXPECT_NE(hash1, hash2);
}

TEST_F(PasswordHasherTest, MD5_SpecialCharacters_ReturnsValidHash)
{
    const auto hash{ utils::PasswordHasher::md5(SPECIAL_CHARS_PASSWORD) };
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 32); // MD5 is always 32 hex characters
}

TEST_F(PasswordHasherTest, MD5_LongInput_ReturnsValidHash)
{
    const auto hash{ utils::PasswordHasher::md5(LONG_PASSWORD) };
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 32);
}

TEST_F(PasswordHasherTest, SHA256_ValidInput_ReturnsCorrectHash)
{
    EXPECT_EQ(utils::PasswordHasher::sha256("abc"), SHA256_ABC);
    EXPECT_EQ(utils::PasswordHasher::sha256(""), SHA256_EMPTY);
}

TEST_F(PasswordHasherTest, SHA256_SameInput_ReturnsSameHash)
{
    const auto hash1{ utils::PasswordHasher::sha256(TEST_PASSWORD) };
    const auto hash2{ utils::PasswordHasher::sha256(TEST_PASSWORD) };
    EXPECT_EQ(hash1, hash2);
    EXPECT_FALSE(hash1.empty());
}

TEST_F(PasswordHasherTest, SHA256_DifferentInput_ReturnsDifferentHash)
{
    const auto hash1{ utils::PasswordHasher::sha256("password1") };
    const auto hash2{ utils::PasswordHasher::sha256("password2") };
    EXPECT_NE(hash1, hash2);
}

TEST_F(PasswordHasherTest, SHA256_SpecialCharacters_ReturnsValidHash)
{
    const auto hash{ utils::PasswordHasher::sha256(SPECIAL_CHARS_PASSWORD) };
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 64); // SHA256 is always 64 hex characters
}

TEST_F(PasswordHasherTest, SHA256_LongInput_ReturnsValidHash)
{
    const auto hash{ utils::PasswordHasher::sha256(LONG_PASSWORD) };
    EXPECT_FALSE(hash.empty());
    EXPECT_EQ(hash.length(), 64);
}

TEST_F(PasswordHasherTest, HashPassword_EmptySalt_UsesMD5)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, EMPTY_STRING) };
    const auto expected{ utils::PasswordHasher::md5(TEST_PASSWORD) };
    EXPECT_EQ(hash, expected);
}

TEST_F(PasswordHasherTest, HashPassword_WithSalt_UsesSHA256)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT) };
    const auto expected{ utils::PasswordHasher::sha256(TEST_PASSWORD + TEST_SALT) };
    EXPECT_EQ(hash, expected);
}

TEST_F(PasswordHasherTest, HashPassword_EmptyPassword_ThrowsException)
{
    EXPECT_THROW(
        const auto result{ utils::PasswordHasher::hashPassword(EMPTY_STRING, TEST_SALT) }, 
        std::invalid_argument
    );
}

TEST_F(PasswordHasherTest, HashPassword_SpecialCharacters_WorksCorrectly)
{
    EXPECT_NO_THROW({
        const auto hash{ utils::PasswordHasher::hashPassword(SPECIAL_CHARS_PASSWORD, TEST_SALT) };
        EXPECT_FALSE(hash.empty());
    });
}

TEST_F(PasswordHasherTest, HashPassword_LongPassword_WorksCorrectly)
{
    EXPECT_NO_THROW({
        const auto hash{ utils::PasswordHasher::hashPassword(LONG_PASSWORD, TEST_SALT) };
        EXPECT_FALSE(hash.empty());
    });
}

TEST_F(PasswordHasherTest, IsPasswordValid_CorrectPasswordWithSalt_ReturnsTrue)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(TEST_PASSWORD, hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, IsPasswordValid_CorrectPasswordWithoutSalt_ReturnsTrue)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, EMPTY_STRING) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(TEST_PASSWORD, hash, EMPTY_STRING));
}

TEST_F(PasswordHasherTest, IsPasswordValid_WrongPassword_ReturnsFalse)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT) };
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid("wrongPassword", hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, IsPasswordValid_WrongSalt_ReturnsFalse)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT) };
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid(TEST_PASSWORD, hash, "wrongSalt"));
}

TEST_F(PasswordHasherTest, IsPasswordValid_EmptyPassword_ReturnsFalse)
{
    const auto hash{ utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT) };
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid(EMPTY_STRING, hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, IsPasswordValid_EmptyHash_ReturnsFalse)
{
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid(TEST_PASSWORD, EMPTY_STRING, TEST_SALT));
}

TEST_F(PasswordHasherTest, IsPasswordValid_BothEmpty_ReturnsFalse)
{
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid(EMPTY_STRING, EMPTY_STRING, EMPTY_STRING));
}

TEST_F(PasswordHasherTest, IsPasswordValid_SpecialCharacters_WorksCorrectly)
{
    const auto hash{ utils::PasswordHasher::hashPassword(SPECIAL_CHARS_PASSWORD, TEST_SALT) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(SPECIAL_CHARS_PASSWORD, hash, TEST_SALT));
    EXPECT_FALSE(utils::PasswordHasher::isPasswordValid("different", hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, EdgeCase_SingleCharacterPassword)
{
	const std::string singleChar{ "a" };
    const auto hash{ utils::PasswordHasher::hashPassword(singleChar, TEST_SALT) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(singleChar, hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, EdgeCase_WhitespacePassword)
{
    const std::string whitespace{ "   " };
    const auto hash{ utils::PasswordHasher::hashPassword(whitespace, TEST_SALT) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(whitespace, hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, EdgeCase_NullCharactersInPassword)
{
    const std::string withNulls{ "pass\0word", 9 };
    const auto hash{ utils::PasswordHasher::hashPassword(withNulls, TEST_SALT) };
    EXPECT_TRUE(utils::PasswordHasher::isPasswordValid(withNulls, hash, TEST_SALT));
}

TEST_F(PasswordHasherTest, Consistency_MultipleCallsSameResult)
{
    std::vector<std::string> hashes;
    hashes.reserve(10);

    for (int i = 0; i < 10; ++i) 
    {
        hashes.push_back(utils::PasswordHasher::hashPassword(TEST_PASSWORD, TEST_SALT));
    }

    for (size_t i = 1; i < hashes.size(); ++i) 
    {
        EXPECT_EQ(hashes[0], hashes[i]);
    }
}

TEST_F(PasswordHasherTest, OutputFormat_MD5_Is32HexCharacters)
{
    const auto hash{ utils::PasswordHasher::md5(TEST_PASSWORD) };
    EXPECT_EQ(hash.length(), 32);
    EXPECT_TRUE(std::all_of(hash.begin(), hash.end(), [](char c)
    {
        return std::isxdigit(static_cast<unsigned char>(c));
    }));
}

TEST_F(PasswordHasherTest, OutputFormat_SHA256_Is64HexCharacters)
{
    const auto hash{ utils::PasswordHasher::sha256(TEST_PASSWORD) };
    EXPECT_EQ(hash.length(), 64);
    EXPECT_TRUE(std::all_of(hash.begin(), hash.end(), [](char c) 
    {
        return std::isxdigit(static_cast<unsigned char>(c));
    }));
}
}

#endif // PasswordHasherTests_h
