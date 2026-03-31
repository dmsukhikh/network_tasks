#include "StreamSocket.hpp"
#include "defs.hpp"
#include <unistd.h>
#include <sys/socket.h>

StreamSocket::StreamSocket() : fd_(-1) {}

StreamSocket::StreamSocket(int fd) : fd_(fd) {}

StreamSocket::~StreamSocket() {
    close();
}

StreamSocket::StreamSocket(StreamSocket&& other) noexcept
    : fd_(other.fd_) {
    other.fd_ = -1;
}

StreamSocket& StreamSocket::operator=(StreamSocket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

ssize_t StreamSocket::send(const Message& msg) {
    return ::send(fd_, &msg, sizeof(msg), 0); 
}

Message StreamSocket::receive() {
    Message buf;
    auto bytes = recv(fd_, &buf, sizeof(Message), 0);
    if (bytes == 0)
    {
        return {0, MSG_BYE};
    }
    return buf;
}

void StreamSocket::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}
