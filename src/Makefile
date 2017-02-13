CC = g++
CFLAGS = -g -Wall -std=c++11

all: miProxy nameserver

miProxy: miProxy.cpp
	$(CC) $(CFLAGS) -o $@ $^

nameserver: nameserver.cpp
	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -f *.o *~ *core* miProxy nameserver


#all : nameserver client

#nameserver: nameserver.cpp
#	$(CC) $(CFLAGS) -o $@ $^

#client: client.cpp
#	$(CC) $(CFLAGS) -o $@ $^

# clean:
# 	-rm -f *.o *~ *core* nameserver client


