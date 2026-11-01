#include <asio.hpp>
#include <iostream>
#include <string>
#include "protocol.h"
#include <thread>
#include <functional>
#include <atomic>
using asio::ip::tcp;

// Prints e.g. "Ana rolled 4d8-2: [1, 7, 1, 3] - 2 = 10".
void print_roll(const RollResultMessage& roll) {
    std::cout << roll.player << " rolled " << roll.expr << ": [";
    for (std::size_t i = 0; i < roll.dice.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << roll.dice[i];
    }
    std::cout << "]";
    if (roll.modifier > 0) {
        std::cout << " + " << roll.modifier;
    } else if (roll.modifier < 0) {
        std::cout << " - " << -roll.modifier;
    }
    std::cout << " = " << roll.total << std::endl;
}

void reader_loop(tcp::socket& socket, std::atomic<bool>& running) {
    asio::error_code ec;
    std::string buf;
    for (;;) {
        std::size_t bytes_transferred = asio::read_until(socket, asio::dynamic_buffer(buf), '\n', ec);
        if (ec) {
            if (running.load()) {
                std::cout << "Server closed the connection. Press enter to exit" << std::endl;
            }
            running.store(false);
            break;
        }
        std::string line = buf.substr(0, bytes_transferred);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        auto msg = parse_server_message(line);
        if (!msg) {
            std::cout << "Server: [" << line << "]" << std::endl;
        } else if (auto rptr = std::get_if<RollResultMessage>(&*msg)) {
            print_roll(*rptr);
        } else if (auto cptr = std::get_if<ChatMessage>(&*msg)) {
            std::cout << cptr->player << ": " << cptr->text << std::endl;
        } else if (auto eptr = std::get_if<ErrorMessage>(&*msg)) {
            std::cout << "! " << eptr->message << std::endl;
        } else if (auto jptr = std::get_if<PlayerJoined>(&*msg)) {
            std::cout << "* " << jptr->name << " joined" << std::endl;
        }
        // Always consume the line, even if it didn't parse, or the next
        // read_until would hand back the same line forever.
        buf.erase(0, bytes_transferred);
    }
}

int main() {
    asio::io_context io_context;
    tcp::socket socket(io_context);
    asio::error_code ec;
    tcp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 5000);
    socket.connect(endpoint, ec);
    if (ec) {
        std::cerr << "Error connecting to server: " << ec.message() << std::endl;
        return 1;
    }
    std::string line;
    std::cout << "Enter your name: ";
    std::getline(std::cin, line);
    JoinRequest joinRequest{line};
    std::string joinMessage = to_line(joinRequest);
    asio::write(socket, asio::buffer(joinMessage), ec);
    if (ec) {
        std::cerr << "Error sending join request: " << ec.message() << std::endl;
        return 1;
    }
    std::atomic<bool> running{true};
    auto reader = std::jthread(reader_loop, std::ref(socket), std::ref(running));
    std::cout << "Commands: /help for help, /quit to quit, /roll <dice_expression>, or type anything to chat."
              << std::endl;

    while (std::getline(std::cin, line)) {
        if (!running.load()) {
            break;
        }
        if (line.empty()) {
            continue;

        } else if (line == "/quit") {
            break;
        } else if (line == "/help") {
            std::cout << "Commands: /roll <dice_expression>, /quit, or type anything to chat." << std::endl;
        } else if (line == "/roll") {
            std::cout << "Usage: /roll <dice_expression>" << std::endl;
        } else if (line.starts_with("/roll ")) {
            std::string expr = line.substr(6);
            RollRequest rollRequest{expr};
            std::string rollMessage = to_line(rollRequest);
            asio::write(socket, asio::buffer(rollMessage), ec);
            if (ec) {
                std::cerr << "Error sending roll request: " << ec.message() << std::endl;
                return 1;
            }
        } else {
            ChatRequest chatRequest{line};
            std::string chatMessage = to_line(chatRequest);
            asio::write(socket, asio::buffer(chatMessage), ec);
            if (ec) {
                std::cerr << "Error sending chat message: " << ec.message() << std::endl;
                return 1;
            }
        }
    }
    running.store(false);
    socket.shutdown(tcp::socket::shutdown_both, ec);
    return 0;
}
