#ifndef UUID_UTILS_H
#define UUID_UTILS_H

#include <string>
#include <boost/uuid/uuid_generators.hpp>

namespace utils
{
/**
 * @class UUIDUtils
 * @brief Utility class for generating and validating UUIDs
 *
 * Provides static methods for generating random UUIDs and validating
 * UUID strings. Uses Boost.UUID library for implementation.
 *
 * @note All methods are thread-safe (using thread-safe Boost generators)
 */
class UUIDUtils final
{
public:
    /**
     * @brief Deleted default constructor
     * @note This is a utility class with only static methods
     */
    UUIDUtils() noexcept = delete;

    /**
     * @brief Generates a random UUID (version 4)
     * @return std::string String representation of the generated UUID
     * @note Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx (RFC 4122)
     */
    static [[nodiscard]] std::string generateUUID() noexcept;

    /**
     * @brief Validates if a string is a properly formatted UUID
     * @param uuid String to validate as UUID
     * @return bool True if string is a valid UUID, false otherwise
     * @note Accepts both uppercase and lowercase hexadecimal digits
     */
    static [[nodiscard]] bool isValidUUID(const std::string& uuid) noexcept;

private:
    static inline boost::uuids::string_generator stringGenerator_; ///< Static generator for string-to-UUID conversion
};
}

#endif // UUID_UTILS_H