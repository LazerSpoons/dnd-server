add_executable(server main.cpp)
target_link_libraries(server PRIVATE
    common
    asio::asio
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
)