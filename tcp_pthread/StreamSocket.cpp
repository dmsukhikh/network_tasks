#include "StreamSocket.hpp"
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

ssize_t StreamSocket::send(const void* buf, size_t len, int flags) {
    return ::send(fd_, buf, len, flags);
}

ssize_t StreamSocket::receive(void* buf, size_t len, int flags) {
    return ::recv(fd_, buf, len, flags);
}

void StreamSocket::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}
