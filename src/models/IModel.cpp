#include "IModel.h"
#include <chrono>

namespace models
{
std::string IModel::getCurrentTimestamp() noexcept
{
    try 
    {
        const auto now{ std::chrono::system_clock::now() };
        return std::format("{0:%Y-%m-%d %H:%M:%S}", now);

    }
    catch (...) 
    {
        return "1970-01-01 00:00:00";
    }
}
}
