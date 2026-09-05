#include <asio.hpp>
#include <nlohmann/json.hpp>
#include <sqlite3.h>
#include <iostream>

int main() {
    asio::io_context io;
    nlohmann::json j = {{"status", "ok"}};
    std::cout << j.dump() << "\n";
    std::cout << "sqlite " << sqlite3_libversion() << "\n";
    return 0;
}