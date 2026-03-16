#include "Server.hpp"
#include <iostream>

#define BUFFER_SIZE 1024

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Not enough args! Usage: ./server [port]" << std::endl;
        return 1;
    }

    try {
        Server server(argv[1]);
        
        server.send();
        
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
