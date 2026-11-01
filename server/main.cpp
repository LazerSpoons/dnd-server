#include <asio.hpp>
#include <string>
#include <utility>
#include <thread>
#include <iostream>
#include <vector>
#include "protocol.h"
#include "dice.h"
using asio::ip::tcp;
void handle_client(tcp::socket socket) {
    asio::error_code ec;
    tcp::endpoint endpoint = socket.remote_endpoint(ec);
    if (ec) {
        std::cerr << "Error getting remote endpoint: " << ec.message() << std::endl;
        return;
    }
    std::cout << "Client connected: " << endpoint << std::endl;
    std::string buf;
    std::string name;
    for (;;) {
        std::size_t bytes_transferred = asio::read_until(socket, asio::dynamic_buffer(buf), '\n', ec);
        if (ec == asio::error::eof) {
            break;
        } else if (ec) {
            std::cerr << "Error reading from socket: " << endpoint << ": " << ec.message() << std::endl;
            break;
        }

        std::string line = buf.substr(0, bytes_transferred);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::cout << "Client " << endpoint << ": [" << line << "]" << std::endl;
        buf.erase(0, bytes_transferred);
        std::string reply;
        auto msg = parse_client_message(line);
        if (!msg) {
            reply = to_line(ErrorMessage{"Invalid message format"});
        } else {
            if (auto jptr = std::get_if<JoinRequest>(&*msg)) {
                std::string newName = jptr->name;
                if (newName.empty()) {
                    reply = to_line(ErrorMessage{"Name cannot be empty"});
                } else if (!name.empty()) {
                    reply = to_line(ErrorMessage{"You have already joined as " + name});
                } else {
                    name = newName;
                    reply = to_line(PlayerJoined{name});
                }
            } else if (auto rptr = std::get_if<RollRequest>(&*msg)) {
                if (name.empty()) {
                    reply = to_line(ErrorMessage{"You must join before rolling"});
                } else {
                    auto result = roll(rptr->expr);
                    if (!result) {
                        reply = to_line(ErrorMessage{"Invalid dice expression"});
                    } else {
                        reply =
                            to_line(RollResultMessage{name, rptr->expr, result->dice, result->modifier, result->total});
                    }
                }

            } else if (auto cptr = std::get_if<ChatRequest>(&*msg)) {
                if (name.empty()) {
                    reply = to_line(ErrorMessage{"You must join before chatting"});
                } else if (cptr->text.empty()) {
                    reply = to_line(ErrorMessage{"Chat message cannot be empty"});
                } else {
                    reply = to_line(ChatMessage{name, cptr->text});
                }
            } else {
                reply = to_line(ErrorMessage{"Unsupported message type"});
            }
        }
        asio::write(socket, asio::buffer(reply), ec);
        if (ec) {
            std::cerr << "Error writing to socket: " << endpoint << ": " << ec.message() << std::endl;
            break;
        }
    }
    std::cout << "Client disconnected: " << endpoint << std::endl;
}
int main() {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 5000));
    std::cout << "Server is listening on 5000..." << std::endl;
    std::vector<std::jthread> threads;
    for (;;) {
        tcp::socket socket(io);
        asio::error_code ec;
        acceptor.accept(socket, ec);
        if (ec) {
            std::cerr << "Error accepting connection: " << ec.message() << std::endl;
            continue;
        }
        threads.emplace_back(handle_client, std::move(socket));
    }
    return 0;
}
