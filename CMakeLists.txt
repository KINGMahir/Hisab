cmake_minimum_required(VERSION 3.20)
project(POSInventory VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

add_executable(pos_inventory
    src/legacy/main.cpp
)

target_link_libraries(pos_inventory PRIVATE SQLite::SQLite3)

target_compile_options(pos_inventory PRIVATE
    -Wall -Wextra -Wpedantic
)

enable_testing()
