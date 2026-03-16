#ifndef SERVER_HPP
#define SERVER_HPP

#include "StreamSocket.hpp"
#include <string>

class Server {
public:
    Server(const std::string& port, int max_connections = 1);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&& other) noexcept;
    Server& operator=(Server&& other) noexcept;

    void send();
    void receive();

    void close();

private:
    StreamSocket accept_connection();

    static const int max_connections_ = 1; 
    StreamSocket listening_socket_, client_socket_;
    std::string port_;
};

#endif // SERVER_HPP
