#include "Client.hpp"
#include "defs.hpp"
#include <iostream>
#include <cstring>
#include <sstream>

int state = 0;

Message makeMessage()
{
    std::string s, cmd;
    std::getline(std::cin, s);
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
    msg.type = MSG_TEXT;
    strncpy(msg.payload, s.c_str(), MAX_PAYLOAD-1);
    msg.payload[MAX_PAYLOAD-1] = '\0';
    return msg;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Not enough args! Usage: ./client [server] [port]" << std::endl;
        return 1;
    }

    try
    {
        Client client(argv[1], argv[2]);
        bool is_running = true;
        while (is_running)
        {
            switch (state)
            {
            case 0:
            {
                Message msg{0, MSG_HELLO};
                std::string nick;
                std::cout << "Enter nickname: ";
                std::getline(std::cin, nick);
                strncpy(msg.payload, nick.c_str(), MAX_PAYLOAD);
                client.send(msg);
                msg = client.receive();
                if (msg.type == MSG_BYE)
                {
                    is_running = false;
                }
                else
                {
                    std::cout << "reply from server: " << msg.payload << std::endl;
                    state = 1;
                }
                break;
            }

            case 1:
            {
                client.send(makeMessage());
                auto msg = client.receive();

                if (msg.type == MSG_PONG)
                {
                    std::cout << "Got pong from server!" << std::endl;
                }
                else if (msg.type == MSG_BYE)
                {
                    is_running = false;
                }
                break;
            }

            default:
                throw std::runtime_error(
                    "Invalid state of the client (see ServerRealisation "
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

