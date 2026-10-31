#include <asio.hpp>

#include <iostream>
using asio::ip::tcp;

int main() {
    asio::io_context io;
    tcp::acceptor acceptor(io, tcp::endpoint(asio::ip::make_address("127.0.0.1"), 5000));
    tcp::socket socket(io);
    std::cout << "Server is listening on 5000..." << std::endl;
    acceptor.accept(socket);
    std::cout << "Client connected: " << socket.remote_endpoint() << std::endl;
    return 0;
}