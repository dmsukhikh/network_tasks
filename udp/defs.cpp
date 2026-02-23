#include "defs.hpp"
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Конфигурируем сокет udp-сервера
 */
void prepare_addrinfo(struct addrinfo* info)
{
    memset(info, 0, sizeof(struct addrinfo));
    info->ai_family = AF_UNSPEC;
    info->ai_socktype = SOCK_DGRAM;
    info->ai_flags = AI_PASSIVE;
}

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


int get_server_socket(const char* port)
{
    int status, sock_fd;
    struct addrinfo server_socket_info;
    struct addrinfo *getaddr_res, *i;
    prepare_addrinfo(&server_socket_info);
    if ((status = getaddrinfo(NULL, port, &server_socket_info, &getaddr_res))
        != 0)
    {
        std::cerr << "Ошибка в getaddrinfo: " << gai_strerror(status)
                  << std::endl;
        exit(1);
    }

    for (i = getaddr_res; i != NULL; i = i->ai_next)
    {
        if ((sock_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol))
            == -1)
        {
            perror("Ошибка: socket: ");
            continue;
        }

        int bob = 1;
        setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &bob, sizeof(bob));

        if (bind(sock_fd, i->ai_addr, i->ai_addrlen) == -1)
        {
            close(sock_fd);
            perror("Ошибка: bind: ");
            continue;
        }
        break;
    }

    if (i == NULL)
    {
        std::cerr << "Сокет невозможно открыть :(" << std::endl;
        freeaddrinfo(getaddr_res);
        exit(1);
    }
    freeaddrinfo(getaddr_res);

    return sock_fd;
}

struct server_info get_client_socket(
    const char* server_addr, const char* server_port)
{
    struct server_info out;

    int sockfd;
    struct addrinfo socket_info, *servinfo, *i;
    int status;

    memset(&socket_info, 0, sizeof socket_info);
    socket_info.ai_family = AF_INET;
    socket_info.ai_socktype = SOCK_DGRAM;

    if ((status
            = getaddrinfo(server_addr, server_port, &socket_info, &servinfo))
        != 0)
    {
        std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
        exit(1);
    }

    for (i = servinfo; i != NULL; i = i->ai_next)
    {
        if ((sockfd = socket(i->ai_family, i->ai_socktype, i->ai_protocol))
            == -1)
        {
            perror("socket: ");
            continue;
        }
        break;
    }

    if (i == NULL)
    {
        std::cerr << "Сокет невозможно открыть :(" << std::endl;
        freeaddrinfo(servinfo);
        exit(1);
    }

    out.fd = sockfd;
    memcpy(&out.addr, i->ai_addr, sizeof(struct sockaddr));
    out.addrlen = i->ai_addrlen;

    freeaddrinfo(servinfo);
    return out;
}
