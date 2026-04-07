#include "Server.hpp"
#include "defs.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <memory>
#include <netdb.h>
#include <unistd.h>

std::pair<std::string, std::string> decodePrivateMsg(const std::string& payload)
{
    auto i = payload.find(':');
    return { std::string(payload.begin(), payload.begin() + i),
        std::string(payload.begin() + i + 1, payload.end()) };
}

std::string encodePrivateMsg(const std::pair<std::string, std::string>& dat)
{
    return dat.first + ":" + dat.second;
}

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
        auto client = std::make_shared<StreamSocket>(accept_connection_());
        pool_.enqueue(std::move(client));
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
                auto msg = conn->receive();

                // Проверка на заполненность комнаты: если мест нет,
                // обрываем соединение
                if (conns_.get()->size() >= max_connections_)
                {
                    conn->send(makeMessageFromText(
                        MSG_ERROR, "Room is full, try later"));
                    is_running = false;
                    break;
                }

                if (msg.type != MSG_HELLO)
                {
                    conn->send(makeMessageFromText(
                        MSG_ERROR, "Invalid protocol! Expected MSG_HELLO"));
                    is_running = false;
                    break;
                }
                else
                {
                    conn->send(makeMessageFromText(MSG_WELCOME, ""));
                    state = 1;
                }

                break;
            }

            case 1:
            {
                // Получаем MSG_AUTH
                auto msg = conn->receive();
                std::string nickname = msg.payload, greating;

                if (msg.type != MSG_AUTH)
                {
                    msg = makeMessageFromText(MSG_ERROR, "Invalid protocol!");
                    is_running = false;
                }
                else if (conns_.get()->count(nickname))
                {
                    msg = makeMessageFromText(MSG_ERROR,
                        nickname + " is used yet. Please choose another one!");
                    is_running = false;
                }
                else if (nickname.size() == 0)
                {
                    msg = makeMessageFromText(
                        MSG_ERROR, "Nickname is empty! Set a valid nickname");
                    is_running = false;
                }
                else if (nickname.size() > NICKNAME_MAX_SIZE)
                {
                    msg = makeMessageFromText(MSG_ERROR,
                        "Nickname is too large! Set a valid nickname");
                    is_running = false;
                }
                else
                {
                    msg = makeMessageFromText(
                        MSG_WELCOME, "Hello, " + nickname + "!");
                    (*conns_.get())[nickname] = conn;
                    state = 2;
                    user = nickname;

                    auto brd_msg = makeMessageFromText(
                        MSG_SERVER_INFO, "Say hi to " + nickname + "!");
                    broadcast_msg_(brd_msg, conn);
                }

                conn->send(msg);

                break;
            }

            case 2:
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
                        MSG_SERVER_INFO, user + " disconnected");
                    broadcast_msg_(brd_msg, conn);
                    is_running = false;
                    break;
                }

                else if (msg.type == MSG_PRIVATE)
                {
                    auto dcd = decodePrivateMsg(msg.payload);
                    auto is_sent = private_msg_(
                        makeMessageFromText(MSG_PRIVATE,
                            encodePrivateMsg({ user, dcd.second })),
                        dcd.first);

                    if (!is_sent)
                    {
                        conn->send(makeMessageFromText(MSG_ERROR,
                            "There is no user with name " + dcd.first));
                    }
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

    conns_.get()->erase(user);
}

void Server::broadcast_msg_(const Message& msg, const SharedSocket& source)
{
    auto conns_copy = *conns_.get(); // Копируем массив, чтобы не держать блокировку 
    for (auto &[k,s]: conns_copy)
    {
        if (source && s == source)
            continue;
        s->send(msg);
    }
}

bool Server::private_msg_(const Message& msg, const std::string& user)
{
    if (!conns_.get()->count(user))
    {
        return false;
    }
    
    auto receiver = conns_.get()->at(user);
    receiver->send(msg); // TODO: обработка ошибок транспортного уровня
    return true;
}

