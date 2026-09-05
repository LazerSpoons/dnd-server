add_executable(server main.cpp)
target_link_libraries(server PRIVATE
    common
    asio::asio
    unofficial::sqlite3::sqlite3
)