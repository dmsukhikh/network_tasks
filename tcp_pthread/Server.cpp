#include "Server.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <netdb.h>
#include <unistd.h>
#include "defs.hpp"

MutexedSocket makeMutexedSocket(StreamSocket&& s)
{
    return std::make_shared<Mutexed<StreamSocket>>(std::move(s));
}

Server::Server(const std::string& port, int max_connections)
    : port_(port)
    , max_connections_(max_connections)
    , pool_(max_connections,
          [this](MutexedSocket&& conn) { serve_connection_(std::move(conn)); })
{
    pthread_mutex_init(&cout_mux, nullptr);
    
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
        pool_.enqueue(*conns_.rbegin());
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

void Server::serve_connection_(MutexedSocket&& conn)
{
    bool is_running = true;
    int state = 0;
    std::string user;

    while (is_running)
    {
        try
        {
            switch (state)
            {
            case 0:
            {
                bool process_validating = true;
                while (process_validating)
                {
                    auto msg = conn->get()->receive();
                    if (msg.type != MSG_HELLO)
                    {
                        is_running = false;
                        break;
                    }

                    std::string nickname = msg.payload, greating;
                    std::cout << nickname << std::endl;
                    
                    if (users_.get()->count(nickname))
                    {
                        greating = nickname
                            + " is used yet. Please choose another one!";
                        msg.type = MSG_TEXT;
                    }
                    else
                    {
                        greating = "Hello, " + nickname + "!";
                        users_.get()->insert(nickname);
                        msg.type = MSG_WELCOME;
                        state = 1;
                        process_validating = false;
                        user = nickname;
                        conn->get()->is_auth = true;
                    }

                    strncpy(msg.payload, greating.c_str(), MAX_PAYLOAD - 1);
                    msg.payload[MAX_PAYLOAD - 1] = '\0';
                    conn->get()->send(msg);
                }

                break;
            }

            case 1:
            {
                auto msg = conn->get()->receive();
                if (msg.type == MSG_TEXT)
                {
                    conn->get()->send({ 0, MSG_TEXT });
                }
                else if (msg.type == MSG_PING)
                {
                    const char* pong = "PONG";
                    msg.type = MSG_PONG;
                    strncpy(msg.payload, pong, 5);
                    conn->get()->send(msg);
                }
                else if (msg.type == MSG_BYE)
                {
                    conn->get()->send({ 0, MSG_BYE });
                    is_running = false;
                    break;
                }
                break;
            }

            default:
                throw std::runtime_error(
                    "Invalid state of the server (see ServerRealisation "
                    "for more info)");
            }
        }

        catch (const std::exception& e)
        {
            pthread_mutex_lock(&cout_mux);
            std::cerr << "Error: " << e.what() << std::endl;
            pthread_mutex_unlock(&cout_mux);
            is_running = false;
        }
    }

    conns_.erase(std::remove(conns_.begin(), conns_.end(), conn), conns_.end());
    users_.get()->erase(user);
}

void Server::broadcast_msg_(const Message& msg, const MutexedSocket& source)
{
    for (auto &s: conns_)
    {
        if (source && source == s && !s->get()->is_auth)
            continue;
        s->get()->send(msg);
    }
}
