all: server client

server: server.cpp utils.h
	g++ --std=c++11 server.cpp -o server

client: client.cpp utils.h
	g++ --std=c++11 client.cpp -o client

.PHONY: clean
clean:
	rm server client