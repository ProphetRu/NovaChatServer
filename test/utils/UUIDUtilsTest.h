#ifndef UUIDUtilsTests_h
#define UUIDUtilsTests_h

#include <gtest/gtest.h>

#include "utils/UUIDUtils.h"

#include <regex>
#include <unordered_set>

namespace utils
{
class UUIDUtilsTest : public ::testing::Test
{
protected:
    const std::string VALID_UUID_V4{ "12345678-1234-1234-1234-123456789abc" };
    const std::string VALID_UUID_V4_UPPER{ "12345678-1234-1234-1234-123456789ABC" };
    const std::string VALID_UUID_V4_DIFFERENT{ "aaaaaaaa-aaaa-aaaa-aaaa-aaaaaaaaaaaa" };

    const std::string EMPTY_STRING{ "" };
    const std::string INVALID_SHORT{ "12345678-1234-1234-1234-123456789ab" };
    const std::string INVALID_LONG{ "12345678-1234-1234-1234-123456789abcd" };
    const std::string INVALID_CHARS{ "12345678-1234-1234-1234-123456789abx" };
    const std::string INVALID_FORMAT{ "12345678123412341234123456789abc" };
    const std::string INVALID_DASH_POSITION{ "1234567-81234-1234-1234-123456789abc" };

    const std::regex UUID_V4_PATTERN{ "^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$", std::regex_constants::icase };
};

TEST_F(UUIDUtilsTest, GenerateUUID_ReturnsNonEmptyString)
{
    const auto uuid{ utils::UUIDUtils::generateUUID() };
    EXPECT_FALSE(uuid.empty());
}

TEST_F(UUIDUtilsTest, GenerateUUID_ReturnsValidUUIDFormat)
{
    const auto uuid{ utils::UUIDUtils::generateUUID() };

    EXPECT_EQ(uuid.length(), 36);
    EXPECT_EQ(uuid[8], '-');
    EXPECT_EQ(uuid[13], '-');
    EXPECT_EQ(uuid[18], '-');
    EXPECT_EQ(uuid[23], '-');

    for (char c : uuid) 
    {
        if (c != '-') 
        {
            EXPECT_TRUE(std::isxdigit(static_cast<unsigned char>(c))) << "Invalid character in UUID: " << c;
        }
    }
}

TEST_F(UUIDUtilsTest, GenerateUUID_ReturnsVersion4UUID)
{
    const auto uuid{ utils::UUIDUtils::generateUUID() };
    EXPECT_EQ(uuid[14], '4');

    const auto version_field = std::tolower(uuid[19]);
    EXPECT_TRUE(version_field == '8' || version_field == '9' || version_field == 'a' || version_field == 'b');
}

TEST_F(UUIDUtilsTest, GenerateUUID_ReturnsUniqueUUIDs)
{
    constexpr auto NUM_UUIDS{ 1000 };
    std::unordered_set<std::string> uuid_set;
    uuid_set.reserve(NUM_UUIDS);

    for (int i = 0; i < NUM_UUIDS; ++i) 
    {
        const auto uuid{ utils::UUIDUtils::generateUUID() };
        EXPECT_EQ(uuid_set.find(uuid), uuid_set.end()) << "Duplicate UUID generated: " << uuid;

        uuid_set.insert(uuid);
        EXPECT_TRUE(utils::UUIDUtils::isValidUUID(uuid)) << "Generated UUID is invalid: " << uuid;
    }

    EXPECT_EQ(uuid_set.size(), NUM_UUIDS);
}

TEST_F(UUIDUtilsTest, GenerateUUID_ReturnsLowercaseUUID)
{
    const auto uuid{ utils::UUIDUtils::generateUUID() };

    for (char c : uuid) 
    {
        if (c != '-') 
        {
            EXPECT_FALSE(std::isupper(static_cast<unsigned char>(c))) << "Uppercase character in generated UUID: " << c;
        }
    }
}

TEST_F(UUIDUtilsTest, IsValidUUID_ValidV4UUID_ReturnsTrue)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID(VALID_UUID_V4));
}

TEST_F(UUIDUtilsTest, IsValidUUID_ValidV4UUIDUpperCase_ReturnsTrue)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID(VALID_UUID_V4_UPPER));
}

TEST_F(UUIDUtilsTest, IsValidUUID_ValidV4UUIDDifferent_ReturnsTrue)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID(VALID_UUID_V4_DIFFERENT));
}

TEST_F(UUIDUtilsTest, IsValidUUID_GeneratedUUID_ReturnsTrue)
{
    const auto uuid{ utils::UUIDUtils::generateUUID() };
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID(uuid));
}

TEST_F(UUIDUtilsTest, IsValidUUID_EmptyString_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(EMPTY_STRING));
}

TEST_F(UUIDUtilsTest, IsValidUUID_TooShort_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(INVALID_SHORT));
}

TEST_F(UUIDUtilsTest, IsValidUUID_TooLong_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(INVALID_LONG));
}

TEST_F(UUIDUtilsTest, IsValidUUID_NonHexCharacters_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(INVALID_CHARS));
}

TEST_F(UUIDUtilsTest, IsValidUUID_WrongDashPositions_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(INVALID_DASH_POSITION));
}

TEST_F(UUIDUtilsTest, IsValidUUID_RandomString_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID("this-is-not-a-uuid-string"));
}

TEST_F(UUIDUtilsTest, IsValidUUID_WhitespaceOnly_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID("   "));
}

TEST_F(UUIDUtilsTest, IsValidUUID_WhitespaceAround_ReturnsFalse)
{
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID("  " + VALID_UUID_V4 + "  "));
}

TEST_F(UUIDUtilsTest, EdgeCase_MaximumValidUUID)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("ffffffff-ffff-ffff-ffff-ffffffffffff"));
}

TEST_F(UUIDUtilsTest, EdgeCase_Version1UUID)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("d3924e70-9d8a-11ed-a8fc-0242ac120002"));
}

TEST_F(UUIDUtilsTest, EdgeCase_Version3UUID)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("6fa459ea-ee8a-3ca4-894e-db77e160355e"));
}

TEST_F(UUIDUtilsTest, EdgeCase_Version5UUID)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("74738ff5-5367-5958-9aee-98fffdcd1876"));
}

TEST_F(UUIDUtilsTest, SpecificCase_UUIDWithMixedCase)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("12345678-1234-1234-1234-123456789AbC"));
}

TEST_F(UUIDUtilsTest, SpecificCase_UUIDWithVersion4Indicator)
{
    EXPECT_TRUE(utils::UUIDUtils::isValidUUID("12345678-1234-4123-8123-123456789abc"));
}

TEST_F(UUIDUtilsTest, BoundaryCase_OneCharacterTooShort)
{
    const auto short_uuid{ VALID_UUID_V4.substr(0, 35) };
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(short_uuid));
}

TEST_F(UUIDUtilsTest, BoundaryCase_OneCharacterTooLong)
{
    const auto long_uuid{ VALID_UUID_V4 + "a" };
    EXPECT_FALSE(utils::UUIDUtils::isValidUUID(long_uuid));
}
}

#endif // UUIDUtilsTests_h
