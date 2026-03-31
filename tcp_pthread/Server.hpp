#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "StreamSocket.hpp"
#include "ThreadPool.hpp"
#include <list>
#include <string>
#include <list>
#include <unordered_set>


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

    StreamSocket accept_connection_();
    void serve_connection_(MutexedSocket &&conn);

    /**
     * \note Если указан source, то туда не посылается сообщение msg
     */
    void broadcast_msg_(const Message& msg, const MutexedSocket& source = nullptr);

    ThreadPool pool_;

    const int max_connections_ = 1; 
    StreamSocket listening_socket_;
    std::string port_;

    Mutexed<std::unordered_set<std::string>> users_;

    pthread_mutex_t cout_mux;
};

#endif // SERVER_HPP
