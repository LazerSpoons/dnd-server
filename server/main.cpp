#include <asio.hpp>
#include <string>
#include <utility>
#include <iostream>
using asio::ip::tcp;
void handle_client(tcp::socket socket){
asio::error_code ec;
    tcp::endpoint endpoint = socket.remote_endpoint(ec);
    if (ec) {
        std::cerr << "Error getting remote endpoint: " << ec.message() << std::endl;
        return;
    }
    std::cout << "Client connected: " << endpoint << std::endl;
    std::string buf;
    for (;;) {
        std::size_t bytes_transferred = asio::read_until(socket, asio::dynamic_buffer(buf), '\n', ec);
        if (ec == asio::error::eof) {
            break;
        } else if (ec) {
            std::cerr << "Error reading from socket: " << ec.message() << std::endl;
            break;
        }

        std::string line = buf.substr(0, bytes_transferred);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        std::cout << "[" << line << "]" << std::endl;
        buf.erase(0, bytes_transferred);

        asio::write(socket, asio::buffer(line + "\n"), ec);
        if (ec) {
            std::cerr << "Error writing to socket: " << ec.message() << std::endl;
            break;
        }
    }
    std::cout << "Client disconnected." << std::endl;
}
int main() {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 5000));
    std::cout << "Server is listening on 5000..." << std::endl;
    for (;;) {
        tcp::socket socket(io);
        asio::error_code ec;
        acceptor.accept(socket, ec);
        if (ec) {
            std::cerr << "Error accepting connection: " << ec.message() << std::endl;
            continue;
        }
        handle_client(std::move(socket));}
    return 0;
}
