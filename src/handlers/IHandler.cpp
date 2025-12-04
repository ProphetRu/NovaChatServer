#include "IHandler.h"
#include "../utils/Logger.h"

namespace handlers
{
boost::beast::http::response<boost::beast::http::string_body> IHandler::createSuccessResponse(const nlohmann::json& data, boost::beast::http::status status, const std::string& message) const noexcept
{
    nlohmann::json responseJson{};
    responseJson["status"] = "success";

    if (!message.empty()) 
    {
        responseJson["message"] = message;
    }

    if (!data.empty()) 
    {
        responseJson["data"] = data;
    }

    return createJsonResponse(responseJson, status);
}

boost::beast::http::response<boost::beast::http::string_body> IHandler::createErrorResponse(boost::beast::http::status status, const std::string& errorCode, const std::string& message) const noexcept
{
    nlohmann::json responseJson{};
    responseJson["status"] = "error";
    responseJson["code"] = errorCode;
    responseJson["message"] = message;

    return createJsonResponse(responseJson, status);
}

boost::beast::http::response<boost::beast::http::string_body> IHandler::createJsonResponse(const nlohmann::json& json, boost::beast::http::status status) const noexcept
{
    boost::beast::http::response<boost::beast::http::string_body> response{ status, 11 }; // 11 - HTTP/1.1
    response.set(boost::beast::http::field::content_type, "application/json");
    response.set(boost::beast::http::field::cache_control, "no-cache");
    response.body() = json.dump(4); // pretty print with 4 spaces
    response.prepare_payload();

    setCorsHeaders(response);
    return response;
}

bool IHandler::isJsonBodyValid(const std::string& body, nlohmann::json& json) const noexcept
{
    if (body.empty()) 
    {
        return false;
    }

    try 
    {
        json = nlohmann::json::parse(body);
        return true;
    }
    catch (const nlohmann::json::exception& e) 
    {
        LOG_ERROR("JSON parsing error: " + std::string{ e.what() });
        return false;
    }
}

std::string IHandler::extractBearerToken(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const auto authHeader{ request.find(boost::beast::http::field::authorization) };
    if (authHeader == request.end()) 
    {
        return "";
    }

    const std::string authValue{ authHeader->value() };
    if (authValue.length() <= 7 || authValue.substr(0, 7) != "Bearer ") 
    {
        return "";
    }

    return authValue.substr(7);
}

void IHandler::setCorsHeaders(boost::beast::http::response<boost::beast::http::string_body>& response) const noexcept
{
    response.set(boost::beast::http::field::access_control_allow_origin, "*");
    response.set(boost::beast::http::field::access_control_allow_methods, "GET, POST, PUT, DELETE, OPTIONS");
    response.set(boost::beast::http::field::access_control_allow_headers, "Content-Type, Authorization");
}

bool IHandler::isJsonContentType(const boost::beast::http::request<boost::beast::http::string_body>& request) const noexcept
{
    const auto contentType{ request.find(boost::beast::http::field::content_type) };
    if (contentType == request.end()) 
    {
        return false;
    }

    const std::string contentTypeValue{ contentType->value() };
    return contentTypeValue.find("application/json") != std::string::npos;
}

int IHandler::stringToInt(const std::string& str, int defaultValue) const noexcept
{
    try
    {
        return std::stoi(str);
    }
    catch (const std::invalid_argument&)
    {
        return defaultValue;
    }
}
}
