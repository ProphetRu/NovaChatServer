// additional option /bigobj
// in project properties -> C/C++ -> Command Line -> Additional Options
// to fix "fatal error C1128: number of sections exceeded object file format limit: compile with /bigobj
//
// additional option _WIN32_WINNT=0x0A00
// in project properties -> C/C++ -> Preprocessor -> Preprocessor Definitions
// to target Windows 10 or later for Boost.Asio

#include "utils/PasswordHasherTest.h"
#include "utils/UUIDUtilsTest.h"
#include "utils/ValidatorsTest.h"
#include "utils/SecurityUtilsTest.h"
#include "utils/LoggerTest.h"

#include "auth/JWTManagerTest.h"

#include "config/ConfigManagerTest.h"

#include "models/UserTest.h"
#include "models/MessageTest.h"

#include "database/DatabaseManagerTest.h"

#include "server/RouterTest.h"

#include "handlers/AuthHandlersTest.h"
#include "handlers/UserHandlersTest.h"
#include "handlers/MessageHandlersTest.h"

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
