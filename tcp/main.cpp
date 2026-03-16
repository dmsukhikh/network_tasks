#include "Server.hpp"
#include <iostream>
#include <cstring>

#define BUFFER_SIZE 1024

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Not enough args! Usage: ./server [port]" << std::endl;
        return 1;
    }

    try {
        Server server(argv[1]);
        
        StreamSocket client_socket = server.accept_connection();
        
        char buf[BUFFER_SIZE] = {0};
        
        ssize_t received = client_socket.receive(buf, BUFFER_SIZE, 0);
        if (received > 0) {
            std::cout << "received: " << buf << std::endl;
        }
        
        strcpy(buf, "hi from server!");
        client_socket.send(buf, BUFFER_SIZE, 0);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
