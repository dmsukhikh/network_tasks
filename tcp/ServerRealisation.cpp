#include "Server.hpp"
#include "defs.hpp"
#include <iostream>
#include <cstring>

/**
 * Состояние сервера
 *
 *   * 0 - Сервер только запущен и ожидает MSG_HELLO
 *   * 1 - Сервер связался с клиентом и ожидает запросов
 */
int state = 0;

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Not enough args! Usage: ./server [port]" << std::endl;
        return 1;
    }

    try
    {
        Server server(argv[1]);
        bool is_running = true;
        while (is_running)
        {
            switch (state)
            {
            case 0:
            {
                auto msg = server.receive();
                if (msg.type != MSG_HELLO)
                {
                    is_running = false;       
                    break;
                }

                std::string nickname = msg.payload;
                std::string greating = "Hello, " + nickname + "!";
                msg.type = MSG_WELCOME;

                strncpy(msg.payload, greating.c_str(), MAX_PAYLOAD-1);
                msg.payload[MAX_PAYLOAD-1] = '\0';

                server.send(msg);
                state = 1;
                break;
            }

            case 1:
            {
                auto msg = server.receive();
                if (msg.type == MSG_TEXT)
                {
                    std::cout << msg.payload << std::endl;
                    server.send({0, MSG_TEXT});
                }
                else if (msg.type == MSG_PING)
                {
                    const char* pong = "PONG";
                    msg.type = MSG_PONG;
                    strncpy(msg.payload, pong, 5);
                    server.send(msg);
                }
                else if (msg.type == MSG_BYE)
                {
                    server.send({0, MSG_BYE});
                    is_running = false;
                    break;
                }
                break;
            }

            default:
                throw std::runtime_error(
                    "Invalid state of the server (see ServerRealisation "
                    "for more info)");
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
