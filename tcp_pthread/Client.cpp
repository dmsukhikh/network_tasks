#include "Client.hpp"
#include "StreamSocket.hpp"
#include <sstream>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

MutexedSocket makeMutexedSocket(StreamSocket&& s)
{
    return std::make_shared<Mutexed<StreamSocket>>(std::move(s));
}

Message makeMessage()
{
    std::string s, cmd;
    std::getline(std::cin, s);
    std::stringstream ss(s);
    
    ss >> cmd;

    if (cmd == "/ping")
    {
        return {0, MSG_PING};
    }
    else if (cmd == "/quit")
    {
        return {0, MSG_BYE};
    }

    Message msg{0};
    msg.type = MSG_TEXT;
    strncpy(msg.payload, s.c_str(), MAX_PAYLOAD-1);
    msg.payload[MAX_PAYLOAD-1] = '\0';
    return msg;
}

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

        socket_ = makeMutexedSocket(StreamSocket(sockfd));
        break;
    }

    freeaddrinfo(servinfo);

    if (!socket_->get()->is_valid()) {
        throw std::runtime_error("client: failed to connect");
    }

    if (!authRoutine_())
    {
        return;
    }

    *is_running_.get() = true;

    pthread_create(
        &listening_thread_, nullptr,
        [](void* arg) -> void*
        {
            auto obj = static_cast<Client*>(arg);
            obj->listenRoutine();
            return nullptr;
        },
        this);

    writerRoutine_();
}

Client::~Client() { pthread_join(listening_thread_, nullptr); }

Client::Client(Client&& other) noexcept
    : socket_(std::move(other.socket_))
    , server_address_(std::move(other.server_address_))
    , port_(std::move(other.port_)) {}


Client& Client::operator=(Client&& other) noexcept {
    if (this != &other) {
        socket_ = std::move(other.socket_);
        server_address_ = std::move(other.server_address_);
        port_ = std::move(other.port_);
    }
    return *this;
}

/**
 * \return false - если сервер закрыл подключение
 */
bool Client::authRoutine_()
{
    bool process_validating = true;

    while (process_validating)
    {
        Message msg { 0, MSG_HELLO };
        std::string nick;
        std::cout << "Enter nickname: ";
        std::getline(std::cin, nick);
        strncpy(msg.payload, nick.c_str(), MAX_PAYLOAD);
        socket_->get()->send(msg);

        msg = socket_->get()->receive();
        if (msg.type == MSG_BYE)
        {
            return false;
        }
        else
        {
            std::cout << "reply from server: " << msg.payload << std::endl;
            if (msg.type == MSG_WELCOME)
            {
                process_validating = false;
            }
        }
    }
    return true;
}

void Client::listenRoutine()
{
    while (*is_running_.get())
    {
        auto msg = socket_->get()->receive();

        if (msg.type == MSG_PONG)
        {
            std::cout << "Got pong from server!" << std::endl;
        }
        else if (msg.type == MSG_BYE)
        {
            *is_running_.get() = false;
        }
    }
}

void Client::writerRoutine_()
{
    while (*is_running_.get())
    {
        auto msg = makeMessage();
        std::cout << "bob" << std::endl;
        socket_->get()->send(msg);
        std::cout << "bob is done" << std::endl;
    }
}
