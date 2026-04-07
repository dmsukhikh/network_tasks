#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "Mutexed.hpp"
#include <string>
#include <string_view>
#include "defs.hpp"

class Client {
public:
    Client(const std::string& server_address, const std::string& port);
    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    Client(Client&& other) noexcept;
    Client& operator=(Client&& other) noexcept;

    void send(const Message &msg);

    /**
     * Перенесено в public, чтобы можно было по красоте запустить тред с этим
     * вот
     */
    void listenRoutine();

    Message receive();

private:
    bool authRoutine_();
    void writerRoutine_();

    void formattedPrint(const std::string_view v);

    pthread_t listening_thread_;

    SharedSocket socket_;
    Mutexed<bool> is_running_ {false};
    bool listen_thread_is_init_ = false;

    std::string server_address_;
    std::string port_;
};

#endif // CLIENT_HPP
