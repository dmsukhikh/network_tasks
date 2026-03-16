.PHONY: all clean

all: server

Client.o: Client.cpp Client.hpp defs.hpp
	g++ -c Client.cpp -o Client.o

StreamSocket.o: StreamSocket.cpp StreamSocket.hpp
	g++ -c StreamSocket.cpp -o StreamSocket.o

libstreamsocket.a: StreamSocket.o
	ar rcs libstreamsocket.a StreamSocket.o

Server.o: Server.cpp Server.hpp StreamSocket.hpp defs.hpp
	g++ -c Server.cpp -o Server.o

server: ServerRealisation.cpp Server.o libstreamsocket.a
	g++ ServerRealisation.cpp -o server -L. -lstreamsocket


clean:
	rm -f server *.o *.a
