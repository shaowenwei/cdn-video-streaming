CC = g++
CFLAGS = -g -Wall -std=c++11

all: miProxy #server client

# server: server.cpp
# 	$(CC) $(CFLAGS) -o $@ $^

miProxy: miProxy.cpp
	$(CC) $(CFLAGS) -o $@ $^
	#g++ -std=c++11 -o miProxy.cpp miProxy

# client: client.cpp
# 	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -f *.o *~ *core* miProxy #server client