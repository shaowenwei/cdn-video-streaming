CC = g++
CFLAGS = -g -Wall -std=c++11

all: miProxy nameserver

miProxy: miProxy.cpp
	$(CC) $(CFLAGS) -o $@ $^

nameserver: nameserver.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -f *.o *~ *core* miProxy nameserver

