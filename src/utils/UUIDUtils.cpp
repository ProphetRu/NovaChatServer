#include "UUIDUtils.h"
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid.hpp>

namespace utils
{
std::string UUIDUtils::generateUUID() noexcept
{
    static boost::uuids::random_generator randomGenerator{};
	const auto uuid{ randomGenerator() };
    return boost::uuids::to_string(uuid);
}

bool UUIDUtils::isValidUUID(const std::string& uuid) noexcept
{
    try 
    {
        const auto uuidObj{ stringGenerator_(uuid) };
        return !uuidObj.is_nil();
    }
    catch (const std::exception&) 
    {
        return false;
    }
}
}
