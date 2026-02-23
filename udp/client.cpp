#include "defs.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout
            << "Недостаточно аргументов: нужно указать адрес и порт udp-сервера"
            << std::endl;
        return -1;
    }
    auto server = get_client_socket(argv[1], argv[2]);
    int from_server_fd = get_server_socket(CLIENT_RETURN_SOCKET);
    char from_server_buffer[BUFFER_SIZE];


    std::string buf;
    while (true)
    {
        int numbytes;
        std::getline(std::cin, buf);
        if (buf.size() > BUFFER_SIZE - 1)
        {
            std::cerr << "Можно передать строку не больше, чем "
                      << BUFFER_SIZE - 1 << " символов" << std::endl;
            continue;
        }

        if ((numbytes = sendto(
                 server.fd, buf.c_str(), buf.size(), 0, &server.addr, server.addrlen))
            == -1)
        {
            perror("Ошибка: sendto");
            close(server.fd);
            close(from_server_fd);
            exit(1);
        }

        if (buf == "exit")
            break;

        if (recvfrom(from_server_fd, from_server_buffer, BUFFER_SIZE-1, 0, NULL, NULL) == -1)
        {
            perror("Ошибка: recvfrom");
            close(server.fd);
            close(from_server_fd);
            exit(1);
        }

        std::cout << "Получен ответ от сервера: " << from_server_buffer << std::endl;

    }

    close(server.fd);
    close(from_server_fd);

    return 0;
}
