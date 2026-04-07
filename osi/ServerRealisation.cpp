#include "Server.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        std::cerr << "Not enough args! Usage: ./server <port> <max_connections>" << std::endl;
        return 1;
    }

    try
    {
        int c = std::stoi(argv[2]);
        Server server(argv[1], c);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
