#include "JWTManager.h"
#include "../utils/Logger.h"
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <stdexcept>

namespace auth
{
    constexpr auto ISSUER{ "nova-chat-server" };
    constexpr size_t MIN_SECRET_KEY_LENGTH{ 32 };
    constexpr unsigned int MIN_ACCESS_TOKEN_EXPIRY{ 1 }; // minute
    constexpr unsigned int MAX_ACCESS_TOKEN_EXPIRY{ 525600 }; // year in minutes

JWTManager::JWTManager(const std::string& secretKey, unsigned int accessTokenExpiryMinutes, unsigned int refreshTokenExpiryDays) :
    secretKey_{ secretKey },
    accessTokenExpiryMinutes_{ accessTokenExpiryMinutes },
    refreshTokenExpiryDays_{ refreshTokenExpiryDays }
{
    if (secretKey_.empty())
    {
        throw std::invalid_argument{ "Secret key cannot be empty" };
    }

    if (secretKey_.size() < MIN_SECRET_KEY_LENGTH) 
    {
        LOG_WARNING("JWT secret key is too short (minimum " + std::to_string(MIN_SECRET_KEY_LENGTH) + " characters recommended)");
    }

    if (accessTokenExpiryMinutes_ < MIN_ACCESS_TOKEN_EXPIRY) 
    {
        throw std::invalid_argument{ "Access token expiry too short" };
    }

    if (accessTokenExpiryMinutes_ > MAX_ACCESS_TOKEN_EXPIRY) 
    {
        throw std::invalid_argument{ "Access token expiry too long" };
    }

    LOG_INFO("JWTManager initialized successfully. Access token expiry: " +
        std::to_string(accessTokenExpiryMinutes_) + " minutes, Refresh token expiry: " +
        std::to_string(refreshTokenExpiryDays_) + " days");
}

std::string JWTManager::generateAccessToken(const std::string& userId, const std::string& login) const
{
    if (userId.empty() || login.empty()) 
    {
        throw std::invalid_argument{ "User ID and login cannot be empty" };
    }

    try 
    {
        auto token{ jwt::create()
            .set_issuer(ISSUER)
            .set_type("JWT")
            .set_subject("access")
            .set_payload_claim("userID", jwt::claim(userId))
            .set_payload_claim("login", jwt::claim(login))
            .set_payload_claim("type", jwt::claim(std::string{ "access" }))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(getAccessTokenExpiry())
            .sign(jwt::algorithm::hs256{ secretKey_ }) };

        LOG_DEBUG("Generated access token for user: " + userId + " (" + login + ")");
        return token;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to generate access token: " + std::string{ e.what() });
        throw std::runtime_error{ e.what() };
    }
}

std::string JWTManager::generateRefreshToken(const std::string& userId) const
{
    if (userId.empty()) 
    {
        throw std::invalid_argument{ "User ID cannot be empty" };
    }

    try 
    {
        auto token{ jwt::create()
            .set_issuer(ISSUER)
            .set_type("JWT")
            .set_subject("refresh")
            .set_payload_claim("userID", jwt::claim(userId))
            .set_payload_claim("type", jwt::claim(std::string{ "refresh" }))
            .set_issued_at(std::chrono::system_clock::now())
            .set_expires_at(getRefreshTokenExpiry())
            .sign(jwt::algorithm::hs256{ secretKey_ }) };

        LOG_DEBUG("Generated refresh token for user: " + userId);
        return token;
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Failed to generate refresh token: " + std::string{ e.what() });
        throw std::runtime_error{ e.what() };
    }
}

JWTManager::TokenPayload JWTManager::verifyAndDecode(const std::string& token)
{
    TokenPayload payload{};

    if (token.empty()) 
    {
        throw std::invalid_argument{ "Token is empty" };
    }

    if (isTokenBlacklisted(token)) 
    {
        throw std::invalid_argument{ "Token is blacklisted" };
    }

    try 
    {
        // token decoding and verification
        const auto decoded{ jwt::decode(token) };
        const auto verifier{ jwt::verify()
                              .allow_algorithm(jwt::algorithm::hs256{ secretKey_ })
                              .with_issuer(ISSUER) };

        verifier.verify(decoded);

        // extract claims
        if (decoded.has_payload_claim("userID")) 
        {
            payload.userID = decoded.get_payload_claim("userID").as_string();
        }

        if (decoded.has_payload_claim("login")) 
        {
            payload.login = decoded.get_payload_claim("login").as_string();
        }

        if (decoded.has_payload_claim("type")) 
        {
            payload.type = decoded.get_payload_claim("type").as_string();
        }

        if (decoded.has_expires_at()) 
        {
            payload.expiresAt = decoded.get_expires_at();
        }

        payload.isValid = true;

        LOG_DEBUG("Token verified successfully for user: " + payload.userID);
        return payload;
    }
    catch (const jwt::error::token_verification_exception& e)
    {
        LOG_DEBUG("Token verification failed: " + std::string{ e.what() });
        throw std::runtime_error{ e.what() };
    }
    catch (const std::exception& e) 
    {
        LOG_ERROR("Unexpected error during token verification: " + std::string{ e.what() });
        throw std::runtime_error{ e.what() };
    }
}

std::chrono::system_clock::time_point JWTManager::getTokenExpiry(const std::string& token)
{
    try 
    {
	    if (const auto decoded{ jwt::decode(token) }; decoded.has_expires_at()) 
        {
            return decoded.get_expires_at();
        }

        throw std::runtime_error{ "Token does not have expiration claim" };
    }
    catch (const std::exception& e) 
    {
        throw std::runtime_error{ "Failed to get token expiry: " + std::string{ e.what() } };
    }
}

void JWTManager::addTokenToBlacklist(const std::string& token) noexcept
{
    if (token.empty()) 
    {
        return;
    }

    std::lock_guard lock{ blacklistMutex_ };

    try 
    {
        const auto expiry{ getTokenExpiry(token) };
        blacklistedTokens_[token] = expiry;

        LOG_DEBUG("Token blacklisted, expires at: " +
            std::to_string(std::chrono::duration_cast<std::chrono::seconds>(expiry.time_since_epoch()).count()));

    }
    catch (const std::exception& e) 
    {
        LOG_WARNING("Failed to blacklist token: " + std::string{ e.what() });
    }
}

bool JWTManager::isTokenBlacklisted(const std::string& token) noexcept
{
    std::lock_guard lock{ blacklistMutex_ };

    if (const auto it{ blacklistedTokens_.find(token) }; it != blacklistedTokens_.end()) 
    {
        // check if the token on the blacklist has expired yet
        if (it->second > std::chrono::system_clock::now()) 
        {
            return true;
        }
    }

    return false;
}

void JWTManager::cleanupExpiredBlacklistedTokens() noexcept
{
    std::lock_guard lock{ blacklistMutex_ };

    const auto now{ std::chrono::system_clock::now() };
    const auto initialSize{ blacklistedTokens_.size() };

    for (auto it = blacklistedTokens_.begin(); it != blacklistedTokens_.end(); ) 
    {
        if (it->second <= now) 
        {
            it = blacklistedTokens_.erase(it);
        }
        else 
        {
            ++it;
        }
    }

    if (const auto removed{ initialSize - blacklistedTokens_.size() }; removed > 0) 
    {
        LOG_DEBUG("Cleaned up " + std::to_string(removed) + " expired blacklisted tokens");
    }
}

std::chrono::system_clock::time_point JWTManager::getAccessTokenExpiry() const noexcept
{
    return std::chrono::system_clock::now() + std::chrono::minutes(accessTokenExpiryMinutes_);
}

std::chrono::system_clock::time_point JWTManager::getRefreshTokenExpiry() const noexcept
{
    return std::chrono::system_clock::now() + std::chrono::hours(24 * refreshTokenExpiryDays_);
}
}
