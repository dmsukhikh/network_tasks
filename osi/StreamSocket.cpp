#include "StreamSocket.hpp"
#include "defs.hpp"
#include <pthread.h>
#include <unistd.h>
#include <sys/socket.h>

StreamSocket::StreamSocket() : fd_(-1), send_mut_(new pthread_mutex_t) 
{
    pthread_mutex_init(send_mut_.get(), nullptr);
}

StreamSocket::StreamSocket(int fd)
    : fd_(fd)
    , send_mut_(new pthread_mutex_t)
{
    pthread_mutex_init(send_mut_.get(), nullptr);
}

StreamSocket::~StreamSocket() {
    if (send_mut_)
    {
        pthread_mutex_destroy(send_mut_.get());
    }
    close();
}

StreamSocket::StreamSocket(StreamSocket&& other) noexcept
    : fd_(other.fd_), send_mut_(std::move(other.send_mut_)) {
    other.fd_ = -1;
}

StreamSocket& StreamSocket::operator=(StreamSocket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
        send_mut_ = std::move(other.send_mut_);
    }
    return *this;
}

ssize_t StreamSocket::send(const Message& msg) {
    pthread_mutex_lock(send_mut_.get());
    auto i = ::send(fd_, &msg, sizeof(msg), 0); 
    pthread_mutex_unlock(send_mut_.get());
    return i;
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
