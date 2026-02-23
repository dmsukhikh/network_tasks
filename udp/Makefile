.PHONY: all clean

all: server client 

server: server.cpp defs.o
	g++ server.cpp defs.o -o server

client: client.cpp defs.o
	g++ client.cpp defs.o -o client

defs.o: defs.cpp
	g++ defs.cpp -c 

clean:
	rm -f server client defs.o
