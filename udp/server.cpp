#include "defs.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Укажите порт, на котором открыть udp-сервер" << std::endl;
        return -1;
    }

    struct server_info client_ret;
    client_ret.fd = -1;
    const char* client_addr_str = NULL;


    char* port = argv[1];
    int sock_fd = get_server_socket(port);
    std::cout << "Сервер принимает соединения на порте " << port << std::endl;

    char buffer[BUFFER_SIZE];
    char addr[INET6_ADDRSTRLEN];

    struct sockaddr_storage client_addr;
    socklen_t client_addrlen = sizeof(client_addr);
    while (true)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        if (recvfrom(sock_fd, buffer, BUFFER_SIZE - 1, 0,
                (struct sockaddr*)&client_addr, &client_addrlen)
            == -1)
        {
            perror("recvfrom");
            close(sock_fd);
            return -1;
        }

        if (memcmp(buffer, "exit", 5) == 0)
        {
            close(sock_fd);
            break;
        }

        if (client_addr_str == NULL)
        {
            client_addr_str = inet_ntop(client_addr.ss_family,
                get_in_addr((struct sockaddr*)&client_addr), addr,
                sizeof(addr));
        }

        std::cout << "Сообщение получено от " << client_addr_str << ": "
                  << buffer << std::endl;

        if (client_ret.fd == -1)
        {
            client_ret = get_client_socket(client_addr_str, CLIENT_RETURN_SOCKET);
        }

        if (sendto(client_ret.fd, buffer, BUFFER_SIZE - 1, 0, &client_ret.addr,
                client_ret.addrlen)
            == -1)
        {
            perror("Ошибка: sendto");
            close(client_ret.fd);
            exit(1);
        }
    }
}
