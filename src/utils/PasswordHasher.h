#ifndef PASSWORD_HASHER_H
#define PASSWORD_HASHER_H

#include <string>
#include <openssl/evp.h>

namespace utils
{
/**
 * @class EVP_MD_CTX_Wrapper
 * @brief RAII wrapper for OpenSSL EVP_MD_CTX context
 *
 * Manages the lifecycle of an OpenSSL EVP_MD_CTX context, ensuring proper
 * initialization and cleanup. Provides move semantics but prohibits copying.
 *
 * @note Uses OpenSSL 1.1.0+ API with EVP_MD_CTX_new/EVP_MD_CTX_free
 * @warning Not thread-safe - each instance should be used by single thread
 */
class EVP_MD_CTX_Wrapper final
{
public:
    /**
     * @brief Constructs wrapper with new EVP_MD_CTX context
     * @note Context is allocated using EVP_MD_CTX_new()
     */
    EVP_MD_CTX_Wrapper() noexcept;

    /**
     * @brief Destructor that frees the EVP_MD_CTX context
     */
    ~EVP_MD_CTX_Wrapper() noexcept;

    /**
     * @brief Deleted copy constructor
     * @note EVP_MD_CTX_Wrapper should not be copied
     */
    EVP_MD_CTX_Wrapper(const EVP_MD_CTX_Wrapper&) = delete;

    /**
     * @brief Deleted copy assignment operator
     * @note EVP_MD_CTX_Wrapper should not be copied
     */
    EVP_MD_CTX_Wrapper& operator=(const EVP_MD_CTX_Wrapper&) = delete;

    /**
     * @brief Move constructor
     * @param other Source wrapper to move from
     * @note Transfers ownership of the EVP_MD_CTX context
     */
    EVP_MD_CTX_Wrapper(EVP_MD_CTX_Wrapper&& other) noexcept;

    /**
     * @brief Move assignment operator
     * @param other Source wrapper to move from
     * @return EVP_MD_CTX_Wrapper& Reference to this wrapper
     * @note Transfers ownership of the EVP_MD_CTX context
     */
    EVP_MD_CTX_Wrapper& operator=(EVP_MD_CTX_Wrapper&& other) noexcept;

    /**
     * @brief Gets raw pointer to the underlying EVP_MD_CTX context
     * @return EVP_MD_CTX* Pointer to OpenSSL context (may be null)
     */
    [[nodiscard]] EVP_MD_CTX* get() const noexcept;

    /**
     * @brief Boolean conversion operator
     * @return bool True if context is valid (non-null), false otherwise
     */
    explicit operator bool() const noexcept;

private:
    EVP_MD_CTX* ctx_; ///< Raw pointer to OpenSSL EVP_MD_CTX context
};

/**
 * @class PasswordHasher
 * @brief Provides cryptographic hashing functions for password security
 *
 * Implements password hashing using OpenSSL's EVP interface with support for
 * MD5 (legacy) and SHA-256 algorithms. Includes salt support for enhanced security.
 *
 * @note All methods are thread-safe (use local OpenSSL contexts)
 * @warning MD5 is cryptographically broken - use only for legacy compatibility
 */
class PasswordHasher final
{
public:
    /**
     * @brief Deleted default constructor
     * @note This is a utility class with only static methods
     */
    PasswordHasher() noexcept = delete;

    /**
     * @brief Computes MD5 hash of input string
     * @param input String to hash
     * @return std::string Hexadecimal representation of MD5 hash, empty string on error
     * @warning MD5 is cryptographically broken - avoid for new systems
     */
    static [[nodiscard]] std::string md5(const std::string& input) noexcept;

    /**
     * @brief Computes SHA-256 hash of input string
     * @param input String to hash
     * @return std::string Hexadecimal representation of SHA-256 hash, empty string on error
     */
    static [[nodiscard]] std::string sha256(const std::string& input) noexcept;

    /**
     * @brief Hashes a password with optional salt
     * @param password Plain text password to hash
     * @param salt Optional salt value (empty for unsalted MD5)
     * @return std::string Hashed password
     * @throws std::invalid_argument if password is empty
     * @note Without salt, uses MD5; with salt, uses SHA-256(password + salt)
     */
    static [[nodiscard]] std::string hashPassword(const std::string& password, const std::string& salt = "");

    /**
     * @brief Verifies if a plain text password matches a stored hash
     * @param password Plain text password to verify
     * @param hash Stored password hash to compare against
     * @param salt Salt used when creating the hash (empty for unsalted MD5)
     * @return bool True if password matches hash, false otherwise
     */
    static [[nodiscard]] bool isPasswordValid(const std::string& password, const std::string& hash, const std::string& salt = "") noexcept;

private:
    /**
     * @brief Converts byte array to hexadecimal string
     * @param bytes Pointer to byte array
     * @param length Number of bytes to convert
     * @return std::string Hexadecimal representation
     * @note Internal helper method
     */
    static [[nodiscard]] std::string bytesToHexString(const unsigned char* bytes, size_t length) noexcept;
};
}

#endif // PASSWORD_HASHER_H