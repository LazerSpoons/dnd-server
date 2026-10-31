add_executable(client-cli main.cpp)
target_link_libraries(client-cli PRIVATE
    common
    asio::asio
    nlohmann_json::nlohmann_json
)
