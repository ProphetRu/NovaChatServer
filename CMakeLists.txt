cmake_minimum_required(VERSION 3.31.6)


set(REQUIRED_ARGS CMAKE_TOOLCHAIN_FILE)

foreach(arg IN LISTS REQUIRED_ARGS)
    if(NOT DEFINED ${arg})
        message(FATAL_ERROR "
❌ CONFIGURATION ERROR
================================================================================
The required argument '${arg}' is not specified.
Example:
cmake .. -DCMAKE_TOOLCHAIN_FILE=\"path/to/vcpkg/scripts/buildsystems/vcpkg.cmake\"
================================================================================")
    endif()
endforeach()

#--------------------------------------------------------------------------------------------------
# variables
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/test)

set(PROJECT_NAME NovaChatServer)
set(TEST_NAME Tests)


#--------------------------------------------------------------------------------------------------
# project
project(${PROJECT_NAME})
enable_language(CXX)


#--------------------------------------------------------------------------------------------------
# builds
set(SRC_FILES
	${SRC_DIR}/auth/JWTManager.cpp
	${SRC_DIR}/config/ConfigManager.cpp
	${SRC_DIR}/database/DatabaseManager.cpp
	${SRC_DIR}/handlers/IHandler.cpp
	${SRC_DIR}/handlers/AuthHandlers.cpp
	${SRC_DIR}/handlers/MessageHandlers.cpp
	${SRC_DIR}/handlers/UserHandlers.cpp
	${SRC_DIR}/models/IModel.cpp
	${SRC_DIR}/models/Message.cpp
	${SRC_DIR}/models/User.cpp
	${SRC_DIR}/server/Listener.cpp
	${SRC_DIR}/server/Router.cpp
	${SRC_DIR}/server/Server.cpp
	${SRC_DIR}/server/Session.cpp
	${SRC_DIR}/utils/Logger.cpp
	${SRC_DIR}/utils/PasswordHasher.cpp
	${SRC_DIR}/utils/SecurityUtils.cpp
	${SRC_DIR}/utils/UUIDUtils.cpp
	${SRC_DIR}/utils/Validators.cpp
)

# project
add_executable(${PROJECT_NAME} 
	${SRC_FILES}
	${SRC_DIR}/main.cpp
)

# tests
add_executable(${TEST_NAME} 
	${SRC_FILES}
	${TEST_DIR}/test.cpp
)

# properties
set_target_properties(${PROJECT_NAME} ${TEST_NAME} 
	PROPERTIES
	CXX_STANDARD 23
	CXX_STANDARD_REQUIRED ON
)


#--------------------------------------------------------------------------------------------------
# gtest
#
find_package(GTest CONFIG REQUIRED)

# add src to the include directories for the test target
target_include_directories(${TEST_NAME} 
	PRIVATE
	${SRC_DIR}
	${GTEST_INCLUDE_DIRS}
	${GTEST_LIBRARY} 
	${GTEST_INCLUDE_DIR}
	${GTEST_MAIN_LIBRARY}
)

# libs
SET(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS beast asio uuid program_options algorithm)
find_package(OpenSSL REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)

set(LIBS_LINK
	${BOOST_LIBRARIES} 
	Boost::beast
	Boost::asio
	OpenSSL::SSL
	OpenSSL::Crypto
	Boost::uuid
	nlohmann_json::nlohmann_json
	libpqxx::pqxx
	jwt-cpp::jwt-cpp
	Boost::program_options
	Boost::algorithm
	GTest::gtest 
	GTest::gtest_main 
)

target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBS_LINK})
target_link_libraries(${TEST_NAME} PRIVATE ${LIBS_LINK})


#--------------------------------------------------------------------------------------------------
# compiler settings
if (MSVC)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")

	add_compile_options("/std:c++latest")
	
	target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0A00)
	target_compile_options(${PROJECT_NAME} PRIVATE /bigobj /W4)
	
	target_compile_definitions(${TEST_NAME} PRIVATE _WIN32_WINNT=0x0A00)
	target_compile_options(${TEST_NAME} PRIVATE /bigobj /W4)
else ()
	target_compile_options(${PROJECT_NAME} PRIVATE
		-std=c++23 -Wall -Wextra -pedantic -Werror -Wno-pessimizing-move
	)
	target_compile_options(${TEST_NAME} PRIVATE
		-std=c++23 -Wall -Wextra -pedantic -Werror -Wno-pessimizing-move
	)
endif (MSVC)


#--------------------------------------------------------------------------------------------------
# testing
enable_testing()
include(GoogleTest)
gtest_discover_tests(${TEST_NAME})


#--------------------------------------------------------------------------------------------------
# copying the config file
if(EXISTS "${SRC_DIR}/config.json")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${SRC_DIR}/config.json
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/config.json
        COMMENT "Copying config.json"
    )
else()
    message(WARNING "The file config.json was not found in the path: ${SRC_DIR}/config.json")
endif()

# copying SSL-certs
if(EXISTS "${SRC_DIR}/sslCerts")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${SRC_DIR}/sslCerts
            $<TARGET_FILE_DIR:${PROJECT_NAME}>/sslCerts
        COMMENT "Copying SSL-certs"
    )
else()
    message(WARNING "The sslCerts folder was not found in the path.: ${SRC_DIR}/sslCerts")
endif()