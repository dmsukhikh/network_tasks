#include "Client.hpp"
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

Client::Client(const std::string& server_address, const std::string& port)
    : server_address_(server_address), port_(port) {
    
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(server_address.c_str(), port.c_str(), &hints, &servinfo)) != 0) {
        throw std::runtime_error("getaddrinfo: " + std::string(gai_strerror(rv)));
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        int sockfd;
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            ::close(sockfd);
            continue;
        }

        socket_ = StreamSocket(sockfd);
        break;
    }

    freeaddrinfo(servinfo);

    if (!socket_.is_valid()) {
        throw std::runtime_error("client: failed to connect");
    }

    std::cout << "client connected to " << server_address << ":" << port << std::endl;
}

Client::~Client() {
    close();
}

Client::Client(Client&& other) noexcept
    : socket_(std::move(other.socket_))
    , server_address_(std::move(other.server_address_))
    , port_(std::move(other.port_)) {}

void Client::send(const Message &msg) 
{ 
    socket_.send(msg);
}

Message Client::receive() 
{  
    return socket_.receive();
}

void Client::close() { socket_.close(); }

Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        close();
        socket_ = std::move(other.socket_);
        server_address_ = std::move(other.server_address_);
        port_ = std::move(other.port_);
    }
    return *this;
}
