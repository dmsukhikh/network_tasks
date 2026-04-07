#include "Server.hpp"
#include "defs.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <unistd.h>

Message makeMessageFromText(MessageType type, const std::string& s)
{
    Message msg;
    msg.length = 0;
    msg.type = type;
    strncpy(msg.payload, s.c_str(), MAX_PAYLOAD - 1);
    msg.payload[MAX_PAYLOAD - 1] = '\0';
    return msg; 
}

Server::Server(const std::string& port, int max_connections)
    : port_(port)
    , max_connections_(max_connections)
    , pool_(max_connections + 1,
          [this](SharedSocket&& conn) { serve_connection_(std::move(conn)); })
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

    if (listen(listening_socket_.get_fd(), 10) == -1)
    {
        throw std::runtime_error("listen failed");
    }

    std::cout << "server waiting connection on port " << port_ << "..."
              << std::endl;

    for (;;)
    {
        auto client = accept_connection_();
        conns_.push_back(std::make_shared<StreamSocket>(std::move(client)));
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

void Server::serve_connection_(SharedSocket&& conn)
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
                bool error_auth = false;

                while (process_validating)
                {
                    auto msg = conn->receive();

                    // Проверка на заполненность комнаты: если мест нет,
                    // обрываем соединение
                    if (conns_.size() > max_connections_)
                    {
                        conn->send(makeMessageFromText(
                            MSG_ERROR, "Room is full, try later"));
                        error_auth = true;
                    }

                    if (msg.type != MSG_HELLO)
                    {
                        is_running = false;
                        break;
                    }

                    std::string nickname = msg.payload, greating;
                    
                    if (users_.get()->count(nickname))
                    {
                        msg = makeMessageFromText(MSG_ERROR,
                            nickname
                                + " is used yet. Please choose another one!");
                        error_auth = true;
                    }
                    else if (nickname.size() == 0)
                    {
                        msg = makeMessageFromText(MSG_ERROR,
                            "Nickname is empty! Set a valid nickname");
                        error_auth = true;
                    }
                    else
                    {
                        msg = makeMessageFromText(MSG_WELCOME, "Hello, " + nickname + "!");
                        users_.get()->insert(nickname);
                        state = 1;
                        process_validating = false;
                        user = nickname;
                        conn->is_auth = true;

                        auto brd_msg = makeMessageFromText(
                            MSG_TEXT, "[server]: Say hi to " + nickname + "!");
                        broadcast_msg_(brd_msg, conn);
                    }

                    conn->send(msg);

                    if (error_auth)
                    {
                        process_validating = false;
                        conn->close();
                        break;
                    }
                }

                break;
            }

            case 1:
            {
                auto msg = conn->receive();
                if (msg.type == MSG_TEXT)
                {
                    auto brd_msg = makeMessageFromText(
                        MSG_TEXT, "[" + user + "]: " + msg.payload);
                    broadcast_msg_(brd_msg, conn);
                }
                else if (msg.type == MSG_PING)
                {
                    const char* pong = "PONG";
                    msg.type = MSG_PONG;
                    strncpy(msg.payload, pong, 5);
                    conn->send(msg);
                }
                else if (msg.type == MSG_BYE)
                {
                    conn->send({ 0, MSG_BYE });
                    auto brd_msg = makeMessageFromText(
                        MSG_TEXT, "[server]: " + user + " disconnected");
                    broadcast_msg_(brd_msg, conn);
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
            std::cerr << "Error: " << e.what() << std::endl;
            is_running = false;
        }
    }

    conns_.erase(std::remove(conns_.begin(), conns_.end(), conn), conns_.end());
    users_.get()->erase(user);
}

void Server::broadcast_msg_(const Message& msg, const SharedSocket& source)
{
    for (auto &s: conns_)
    {
        if ((source && s == source) || !s->is_auth)
            continue;
        s->send(msg);
    }
}
