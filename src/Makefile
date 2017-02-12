CC = g++
CFLAGS = -g -Wall -std=c++11

all: miProxy #server client

#server: server.cpp
 #	$(CC) $(CFLAGS) -o $@ $^

miProxy: miProxy.cpp
	$(CC) $(CFLAGS) -o $@ $^

#client: client.cpp
 #	$(CC) $(CFLAGS) -o $@ $^

clean:
 	-rm -f *.o *~ *core* miProxy #server client


#all : nameserver client

#nameserver: nameserver.cpp
#	$(CC) $(CFLAGS) -o $@ $^

#client: client.cpp
#	$(CC) $(CFLAGS) -o $@ $^

#clean:
#	-rm -f *.o *~ *core* nameserver client