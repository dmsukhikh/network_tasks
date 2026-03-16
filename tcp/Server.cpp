#include "Server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>

Server::Server(const std::string& port, int max_connections)
    : port_(port)
{

    struct addrinfo hints, *servinfo, *p;
    int rv;
    int yes = 1;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0)
    {
        throw std::runtime_error(
            "getaddrinfo: " + std::string(gai_strerror(rv)));
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        int sockfd;
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
            == -1)
        {
            ::close(sockfd);
            throw std::runtime_error("setsockopt failed");
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            ::close(sockfd);
            perror("server: bind");
            continue;
        }

        listening_socket_ = StreamSocket(sockfd);
        break;
    }

    freeaddrinfo(servinfo);

    if (!listening_socket_.is_valid())
    {
        throw std::runtime_error("server: failed to bind");
    }

    if (listen(listening_socket_.get_fd(), max_connections_) == -1)
    {
        throw std::runtime_error("listen failed");
    }

    std::cout << "server waiting connection on port " << port_ << "..."
              << std::endl;
}

Server::~Server() { close(); }

Server::Server(Server&& other) noexcept
    : listening_socket_(std::move(other.listening_socket_))
    , port_(std::move(other.port_))
{
}

StreamSocket Server::accept_connection()
{
    struct sockaddr_in con_addr;
    socklen_t con_addr_size = sizeof(con_addr);

    int client_fd = accept(listening_socket_.get_fd(),
        (struct sockaddr*)(&con_addr), &con_addr_size);

    if (client_fd == -1)
    {
        throw std::runtime_error("accept failed");
    }

    return StreamSocket(client_fd);
}

void Server::close() { listening_socket_.close(); }

Server& Server::operator=(Server&& other) noexcept
{
    if (this != &other)
    {
        close();
        listening_socket_ = std::move(other.listening_socket_);
        port_ = std::move(other.port_);
    }
    return *this;
}

void Server::send() { }

void Server::receive() { }
