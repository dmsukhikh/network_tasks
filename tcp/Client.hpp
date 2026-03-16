#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "StreamSocket.hpp"
#include <string>

class Client {
public:
    Client(const std::string& server_address, const std::string& port);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& other) noexcept;
    Client& operator=(Client&& other) noexcept;

    ssize_t send(const void* buf, size_t len, int flags = 0);
    ssize_t receive(void* buf, size_t len, int flags = 0);
    void close();
    bool is_connected() const { return socket_.is_valid(); }

private:
    StreamSocket socket_;
    std::string server_address_;
    std::string port_;
};

#endif // CLIENT_HPP
