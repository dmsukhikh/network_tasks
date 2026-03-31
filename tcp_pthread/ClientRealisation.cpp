#include "Client.hpp"
#include <iostream>
#include <pthread.h>

int state = 0;

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Not enough args! Usage: ./client [server] [port]"
                  << std::endl;
        return 1;
    }

    try
    {
        Client client(argv[1], argv[2]);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
