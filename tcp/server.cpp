#include <iostream>
#include <memory.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#define MAX_CONN 1 // Не меняется
#define BUFFER_SIZE 1024

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Not enough args! Usage: ./server [port]" << std::endl;
        return 1;
    }

    const char* server_port = argv[1];

    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in con_addr;

    int y = 1;
    char s[INET_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo(NULL, server_port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol))
            == -1)
        {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1)
        {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == NULL)
    {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, 1) == -1)
    {
        perror("listen");
        exit(1);
    }

    std::cout << "server waiting connection on port " << server_port << "..."
              << std::endl;

    socklen_t con_addr_size = sizeof con_addr;
    int client_fd
        = accept(sockfd, (struct sockaddr*)(&con_addr), &con_addr_size);
    if (client_fd == -1)
    {
        perror("accept");
        close(sockfd);
        return 1;
    }

    char buf[BUFFER_SIZE];

    // for (;;)
    // {
        ssize_t i = recv(client_fd, buf, BUFFER_SIZE, 0);
        std::cout << "received: " << buf << std::endl; 
        strcpy(buf, "hi from server!");
        i = send(client_fd, buf, BUFFER_SIZE, 0);
        
    // }

    close(sockfd);
    close(client_fd);
}
