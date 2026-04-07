#include "Client.hpp"
#include "StreamSocket.hpp"
#include <sstream>
#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <termios.h>

std::string current_input = ""; 
struct termios orig_termios;

/**
 * Тема для красивого вывода в консоль
 *
 * Написана моим другом из Сан-Диего
 */
void printFormatted(const std::string& msg) {

    // 1. Возврат в начало строки и полная очистка текущей строки (\r\x1b[2K)
    // 2. Печать входящего сообщения и перенос строки (\n), что обеспечит правильный скроллинг!
    std::cout << "\r\x1b[2K" << msg << "\n";

    // 3. Заново отрисовываем приглашение ко вводу и недописанный текст
    std::cout << "> " << current_input;
    std::cout.flush();
}

// Вспомогательная функция настройки терминала
void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    // Отключаем канонический режим (ICANON) и системное эхо (ECHO)
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

Message makeMessage(const std::string& s)
{
    std::string cmd;
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
    if (cmd == "/private")
    {
        msg.type = MSG_PRIVATE;
        std::string to,payload;
        ss >> to;
        std::getline(ss, payload, '\0');
        to += ":" + payload;
        strncpy(msg.payload, to.c_str(), MAX_PAYLOAD-1);
    }
    else
    {
        msg.type = MSG_TEXT;
        strncpy(msg.payload, s.c_str(), MAX_PAYLOAD-1);
    }

    msg.payload[MAX_PAYLOAD-1] = '\0';
    return msg;
}

Client::Client(const std::string& server_address, const std::string& port)
    : server_address_(server_address), port_(port) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(
             server_address.c_str(), port.c_str(), &hints, &servinfo))
        != 0)
    {
        throw std::runtime_error(
            "getaddrinfo: " + std::string(gai_strerror(rv)));
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

        socket_ = std::make_shared<StreamSocket>(sockfd);
        break;
    }

    freeaddrinfo(servinfo);

    if (!socket_->is_valid()) {
        throw std::runtime_error("client: failed to connect");
    }

    if (!authRoutine_())
    {
        return;
    }

    *is_running_.get() = true;

    enableRawMode();

    pthread_create(
        &listening_thread_, nullptr,
        [](void* arg) -> void*
        {
            auto obj = static_cast<Client*>(arg);
            obj->listenRoutine();
            return nullptr;
        },
        this);

    listen_thread_is_init_ = true;

    writerRoutine_();
}

Client::~Client()
{
    if (listen_thread_is_init_)
    {
        pthread_join(listening_thread_, nullptr);
    }
    disableRawMode();
}

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
        socket_->send(msg);

        msg = socket_->receive();
        if (msg.type == MSG_ERROR)
        {
            std::cout << "can't connect to server! reply: " << msg.payload
                      << std::endl
                      << std::endl;
            return false;
        }
        else
        {
            std::cout << "reply from server: " << msg.payload << std::endl
                      << std::endl;

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
        auto msg = socket_->receive();

        if (msg.type == MSG_PONG)
        {
            printFormatted("Got pong from server!");
        }
        else if (msg.type == MSG_BYE)
        {
            *is_running_.get() = false;
            socket_->close();
        }
        else if (msg.type == MSG_TEXT)
        {
            printFormatted(msg.payload);
        }
    }
}

void Client::writerRoutine_()
{
    char c;
    std::cout << "> ";
    std::cout.flush();
    while (read(STDIN_FILENO, &c, 1) == 1 && *is_running_.get()) { 
        if (c == '\n') {
            // Пользователь нажал Enter
            std::cout << "\r\x1b[2K> " << current_input << "\n"; // Фиксируем его ввод
            auto msg = makeMessage(current_input);
            socket_->send(msg);
            if (msg.type == MSG_BYE)
            {
                break;
            }
            current_input.clear();
            std::cout << "> ";
            
        } else if (c == 127) { 
            // Обработка Backspace (в POSIX системах это обычно ASCII 127)
            if (!current_input.empty()) {
                current_input.pop_back();
                std::cout << "\b \b"; // Сдвигаем курсор, стираем пробелом, сдвигаем обратно
            }
        } else {
            // Обычный символ
            current_input += c;
            std::cout << c; // Эхо-вывод
        }
        std::cout.flush();
    }
}
