.PHONY: all clean

all: server client

Client.o: Client.cpp Client.hpp defs.hpp
	g++ -c Client.cpp -o Client.o

StreamSocket.o: StreamSocket.cpp StreamSocket.hpp
	g++ -c StreamSocket.cpp -o StreamSocket.o

libstreamsocket.a: StreamSocket.o
	ar rcs libstreamsocket.a StreamSocket.o

ThreadPool.o: ThreadPool.cpp ThreadPool.hpp libstreamsocket.a
	g++ -c ThreadPool.cpp -o ThreadPool.o -L. -lstreamsocket

Server.o: Server.cpp Server.hpp StreamSocket.hpp defs.hpp
	g++ -c Server.cpp -o Server.o

server: ServerRealisation.cpp Server.o ThreadPool.o libstreamsocket.a
	g++ ServerRealisation.cpp Server.o ThreadPool.o -o server -L. -lstreamsocket

client: ClientRealisation.cpp Client.o libstreamsocket.a
	g++ ClientRealisation.cpp Client.o -o client -L. -lstreamsocket


clean:
	rm -f server client *.o *.a
