#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "StreamSocket.hpp"
#include "ThreadPool.hpp"
#include <list>
#include <string>
#include <list>


class Server {
public:
    Server(const std::string& port, int max_connections = 1);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    Server(Server&& other) noexcept;
    Server& operator=(Server&& other) noexcept;

    void close();

private:
    std::list<MutexedSocket> conns_;

    void serve_connection_(MutexedSocket &&conn);

    StreamSocket accept_connection_();
    ThreadPool pool_;

    const int max_connections_ = 1; 
    StreamSocket listening_socket_;
    std::string port_;
};

#endif // SERVER_HPP
