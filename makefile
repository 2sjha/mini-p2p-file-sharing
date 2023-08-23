CC = g++
CXXFLAGS = --std=c++11 -lpthread

all: p2p-server

debug: CXXFLAGS += -DDEBUG -g
debug: p2p-server

p2p-server: p2p-server.o simple-search.o validations.o utils.o file-sharing.o
	$(CC) $(CXXFLAGS) build/p2p-server.o build/simple-search.o build/file-sharing.o build/validations.o build/utils.o -o p2p-server

p2p-server.o: p2p-server.cpp common.h
	mkdir -p build
	$(CC) $(CXXFLAGS) -c p2p-server.cpp -o build/p2p-server.o

simple-search.o: simple-search.cpp common.h
	$(CC) $(CXXFLAGS) -c simple-search.cpp -o build/simple-search.o

file-sharing.o: file-sharing.cpp common.h
	$(CC) $(CXXFLAGS) -c file-sharing.cpp -o build/file-sharing.o

validations.o: validations.cpp common.h
	$(CC) $(CXXFLAGS) -c validations.cpp -o build/validations.o

utils.o: utils.cpp common.h
	$(CC) $(CXXFLAGS) -c utils.cpp -o build/utils.o

.PHONY: clean
clean:
	rm -rf build
	rm -rf aosp2
	rm p2p-server