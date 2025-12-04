#include "PasswordHasher.h"
#include <ranges>
#include <stdexcept>
#include <openssl/md5.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>

namespace utils
{
constexpr auto OK_CODE{ 1 };

EVP_MD_CTX_Wrapper::EVP_MD_CTX_Wrapper() noexcept:
	ctx_{ EVP_MD_CTX_new() }
{}

EVP_MD_CTX_Wrapper::~EVP_MD_CTX_Wrapper() noexcept
{
    if (ctx_)
    {
        EVP_MD_CTX_free(ctx_);
    }
}

EVP_MD_CTX_Wrapper::EVP_MD_CTX_Wrapper(EVP_MD_CTX_Wrapper&& other) noexcept :
    ctx_{ other.ctx_ }
{
    other.ctx_ = nullptr;
}

EVP_MD_CTX_Wrapper& EVP_MD_CTX_Wrapper::operator=(EVP_MD_CTX_Wrapper&& other) noexcept
{
    if (this != &other)
    {
        if (ctx_)
        {
            EVP_MD_CTX_free(ctx_);
        }

        ctx_ = other.ctx_;
        other.ctx_ = nullptr;
    }

    return *this;
}

EVP_MD_CTX* EVP_MD_CTX_Wrapper::get() const noexcept
{
	return ctx_;
}
EVP_MD_CTX_Wrapper::operator bool() const noexcept
{
	return ctx_ != nullptr;
}

std::string PasswordHasher::md5(const std::string& input) noexcept
{
	const EVP_MD_CTX_Wrapper context{};
    if (!context) 
    {
        return "";
    }

    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int digest_length{};

    if (EVP_DigestInit_ex(context.get(), EVP_md5(), nullptr) != OK_CODE)
    {
        return "";
    }

    if (EVP_DigestUpdate(context.get(), input.c_str(), input.size()) != OK_CODE)
    {
        return "";
    }

    if (EVP_DigestFinal_ex(context.get(), digest, &digest_length) != OK_CODE)
    {
        return "";
    }

    return bytesToHexString(digest, digest_length);
}

std::string PasswordHasher::sha256(const std::string& input) noexcept
{
	const EVP_MD_CTX_Wrapper context{};
    if (!context) 
    {
        return "";
    }

    unsigned char digest[EVP_MAX_MD_SIZE]{};
    unsigned int digest_length{};

    if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != OK_CODE)
    {
        return "";
    }

    if (EVP_DigestUpdate(context.get(), input.c_str(), input.size()) != OK_CODE)
    {
        return "";
    }

    if (EVP_DigestFinal_ex(context.get(), digest, &digest_length) != OK_CODE)
    {
        return "";
    }

    return bytesToHexString(digest, digest_length);
}

std::string PasswordHasher::hashPassword(const std::string& password, const std::string& salt)
{
    if (password.empty()) 
    {
        throw std::invalid_argument{ "Password cannot be empty" };
    }

    if (salt.empty()) 
    {
        return md5(password);
    }
    
    return sha256(password + salt);
}

bool PasswordHasher::isPasswordValid(const std::string& password, const std::string& hash, const std::string& salt) noexcept
{
    if (password.empty() || hash.empty()) 
    {
        return false;
    }

    std::string computedHash;
    if (salt.empty()) 
    {
        computedHash = md5(password);
    }
    else 
    {
        computedHash = sha256(password + salt);
    }

    return computedHash == hash;
}

std::string PasswordHasher::bytesToHexString(const unsigned char* bytes, size_t length) noexcept
{
    std::stringstream ss{};
    ss << std::hex << std::setfill('0');

    for (auto i : std::ranges::views::iota(0u, length))
    {
        ss << std::setw(2) << static_cast<unsigned int>(bytes[i]);
    }

    return ss.str();
}
}