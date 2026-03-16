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


    StreamSocket accept_connection();
    void close();

private:
    StreamSocket listening_socket_;
    std::string port_;
    int max_connections_;
};

#endif // SERVER_HPP
