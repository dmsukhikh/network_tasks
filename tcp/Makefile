.PHONY: all clean

all: main

Client.o: Client.cpp Client.hpp
	g++ -c Client.cpp -o Client.o

StreamSocket.o: StreamSocket.cpp StreamSocket.hpp
	g++ -c StreamSocket.cpp -o StreamSocket.o

libstreamsocket.a: StreamSocket.o
	ar rcs libstreamsocket.a StreamSocket.o

Server.o: Server.cpp Server.hpp StreamSocket.hpp
	g++ -c Server.cpp -o Server.o

main.o: main.cpp Server.hpp
	g++ -c main.cpp -o main.o

main: main.o Server.o Client.o libstreamsocket.a
	g++ -o server main.o Server.o Client.o -L. -lstreamsocket

clean:
	rm -f server *.o *.a
