#include "SecurityUtils.h"
#include "Validators.h"
#include "UUIDUtils.h"

namespace utils
{
std::string SecurityUtils::sanitizeUserInput(const std::string& input) noexcept
{
    if (input.empty()) 
    {
        return input;
    }

    auto sanitized{ Validators::sanitizeString(input) };
    if (sanitized.empty()) 
    {
        return sanitized;
    }

    if (Validators::isSQLInjection(sanitized) || Validators::isXSS(sanitized)) 
    {
        return "";
    }

    return sanitized;
}
}
