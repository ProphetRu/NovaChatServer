# NovaChatServer
**NovaChatServer** - is not a commercial educational project, just for fun.

---

## Dependencies
* [cmake](https://github.com/Kitware/CMake)
* [doxygen](https://github.com/doxygen/doxygen)
* [boost-beast](https://github.com/boostorg/beast)
* [boost-asio](https://github.com/boostorg/asio)
* [openssl](https://github.com/openssl/openssl)
* [boost-uuid](https://github.com/boostorg/uuid)
* [nlohmann-json](https://github.com/nlohmann/json)
* [libpqxx](https://github.com/jtv/libpqxx)
* [jwt-cpp](https://github.com/Thalhammer/jwt-cpp)
* [boost program-options](https://github.com/boostorg/program_options)
* [boost-algorithm](https://github.com/boostorg/algorithm)
* [googletest](https://github.com/google/googletest)

---

## Description
This is an backend REST API server. The server supports endpoints for implementing chat functions.
Key server features:
* Modular project structure;
* Asynchronous multithreaded architecture;
* SSL/TLS support for HTTPS;
* PostgreSQL support with secure connections;
* JWT authentication support;

### The server provides the following features for chat users
#### Authentication and account management
* Registration, login, and logout;
* Access token update via refresh token;
* Password change;
* Account deletion;

#### User search
* View user list with pagination and search;
* Search for users by login;

#### Message management
* Sending messages;
* Receiving messages (with unread filter);
* Marking messages as read;

#### Security and restrictions
* Password (MD5) and token (SHA-256) hashing;
* HTTPS/SSL;
* JWT authentication with token expiration time;
* Protection against web attacks;

#### Configuration
* JSON file with server, SSL, database, JWT, and logging settings;

#### Database
* Tables: `users`, `messages`, `refresh_tokens`;
* Triggers and functions for checking message validity and deleting expired tokens;

![struct](https://raw.githubusercontent.com/ProphetRu/NovaChatServer/master/docs/struct.png)

---

## API
A description of the API look in [this guide](https://github.com/ProphetRu/NovaChatServer/blob/master/docs/API.md).

A description of the API for Swagger Editor [look here](https://github.com/ProphetRu/NovaChatServer/blob/master/docs/swagger.yaml).

---

## How to use the project
### Database setup
* Install the database PostgreSQL;
* Configure a remote connection to the database;
* Generate SSL certificates and set up a secure connection to the database;
* Create a database user;
* Create tables, relationships, functions, triggers using [this guide](https://github.com/ProphetRu/NovaChatServer/blob/master/docs/Database%20schema.md);

---

### Server setup
* Generate SSL certificates for the server;
* Copy the certificates to the `src/sslCerts` folder;
* Edit the config file `src/config.json` using [this guide](https://github.com/ProphetRu/NovaChatServer/blob/master/docs/config.md);

---

### Install dependencies
```shell
vcpkg install boost-beast boost-asio openssl boost-uuid nlohmann-json libpqxx jwt-cpp boost-program-options boost-algorithm gtest
vcpkg integrate install
```

---

### Build the project
Generate cmake files:
```shell
cd NovaChatServer
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE="path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"
```

Build:
```shell
# debug 
cmake --build . 

# release
cmake --build . --config Release
```

---

### Testing
#### gtest
```shell
ctest
```

#### Postman
A collection with tests for Postman [look here](https://github.com/ProphetRu/NovaChatServer/blob/master/docs/NovaChatServer.postman_collection.json).
