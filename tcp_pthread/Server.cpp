#include "Server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <unistd.h>
#include "defs.hpp"

MutexedSocket makeMutexedSocket(StreamSocket&& s)
{
    return std::make_shared<Mutexed<StreamSocket>>(std::move(s));
}

Server::Server(const std::string& port, int max_connections)
    : port_(port), max_connections_(max_connections), pool_(max_connections, nullptr)
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

    for (;;)
    {
        auto client = accept_connection_();
        conns_.push_back(makeMutexedSocket(std::move(client)));
        // pool_.enqueue(std::move(client));
    }
}

Server::~Server() { close(); }

Server::Server(Server&& other) noexcept
    : listening_socket_(std::move(other.listening_socket_))
    , port_(std::move(other.port_))
    , pool_(std::move(other.pool_))
    , max_connections_(std::move(other.max_connections_))
{
}

StreamSocket Server::accept_connection_()
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
        const_cast<int &>(max_connections_) = std::move(other.max_connections_);
    }
    return *this;
}
