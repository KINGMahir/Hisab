cmake_minimum_required(VERSION 3.20)

project(Hisab VERSION 0.3.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(pos_inventory
    src/legacy/main.cpp
    src/database/Database.cpp
    src/database/Migrations.cpp
    src/security/PasswordHasher.cpp
    src/auth/AuthenticationService.cpp
)

target_include_directories(pos_inventory PRIVATE
    src
)

target_link_libraries(pos_inventory PRIVATE
    SQLite::SQLite3
    OpenSSL::Crypto
)

target_compile_options(pos_inventory PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

# Database runtime test
add_executable(database_test
    tests/database/database_test.cpp
    src/database/Database.cpp
    src/database/Migrations.cpp
)

target_include_directories(database_test PRIVATE
    src
)

target_link_libraries(database_test PRIVATE
    SQLite::SQLite3
)

target_compile_options(database_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

# Password hashing test
add_executable(password_hasher_test
    tests/security/password_hasher_test.cpp
    src/security/PasswordHasher.cpp
)

target_include_directories(password_hasher_test PRIVATE
    src
)

target_link_libraries(password_hasher_test PRIVATE
    OpenSSL::Crypto
)

target_compile_options(password_hasher_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)


# Authentication service test
add_executable(authentication_test
    tests/auth/authentication_test.cpp
    src/auth/AuthenticationService.cpp
    src/security/PasswordHasher.cpp
    src/database/Database.cpp
    src/database/Migrations.cpp
)

target_include_directories(authentication_test PRIVATE
    src
)

target_link_libraries(authentication_test PRIVATE
    SQLite::SQLite3
    OpenSSL::Crypto
)

target_compile_options(authentication_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)