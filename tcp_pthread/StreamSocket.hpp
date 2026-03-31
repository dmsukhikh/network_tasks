#ifndef STREAM_SOCKET_HPP
#define STREAM_SOCKET_HPP

#include <sys/types.h>
#include "defs.hpp"

/**
 * Высокоуровневая абстракция над сокетом, работающая с Message
 * Умеет:
 * * Посылать сообщения формата Message
 * * Показывает, авторизовался ли пользователь
 */
class StreamSocket {
public:
    StreamSocket();
    explicit StreamSocket(int fd);
    ~StreamSocket();

    // Запрещаем копирование
    StreamSocket(const StreamSocket&) = delete;
    StreamSocket& operator=(const StreamSocket&) = delete;

    // Разрешаем перемещение
    StreamSocket(StreamSocket&& other) noexcept;
    StreamSocket& operator=(StreamSocket&& other) noexcept;

    ssize_t send(const Message& msg);
    Message receive();

    int get_fd() const { return fd_; }
    bool is_valid() const { return fd_ != -1; }
    void close();

    bool is_auth = false;

private:
    int fd_;
};

#endif // STREAM_SOCKET_HPP
