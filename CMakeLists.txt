cmake_minimum_required(VERSION 3.21)
project(dnd_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Asio needs a target Windows version; 0x0A00 = Windows 10+.
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

find_package(asio CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)
find_package(Catch2 3 CONFIG REQUIRED)

enable_testing()
add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client-cli)
add_subdirectory(tests)