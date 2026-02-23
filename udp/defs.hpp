/**
 * Общие определения и функции
 */
#pragma once
#define BUFFER_SIZE 1024
#define CLIENT_RETURN_SOCKET "22334"
#include <sys/socket.h>

struct server_info
{
    int fd;
    struct sockaddr addr;
    socklen_t addrlen;
};

int get_server_socket(const char* port);
struct server_info get_client_socket(const char* addr, const char* port);
void *get_in_addr(struct sockaddr *sa);
