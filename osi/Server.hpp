#ifndef SERVER_HPP
#define SERVER_HPP

#include "defs.hpp"
#include "StreamSocket.hpp"
#include "ThreadPool.hpp"
#include "Mutexed.hpp"
#include <string>
#include <unordered_map>


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
    // Авторизованные пользователи
    Mutexed<std::unordered_map<std::string, SharedSocket>> conns_;

    StreamSocket accept_connection_();
    void serve_connection_(SharedSocket &&conn);

    /**
     * \note Если указан source, то туда не посылается сообщение msg
     */
    void broadcast_msg_(const Message& msg, const SharedSocket& source = nullptr);

    /**
     * Сообщение msg доставляется только клиенту user
     * \return false, если сообщение не доставлено. Иначе - true;
     */
    bool private_msg_(const Message& msg, const std::string& user);

    ThreadPool pool_;

    const int max_connections_ = 1; 
    StreamSocket listening_socket_;
    std::string port_;
};

#endif // SERVER_HPP
