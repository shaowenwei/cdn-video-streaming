CC = g++
CFLAGS = -g -Wall -std=c++11

all: proxy #server client

# server: server.cpp
# 	$(CC) $(CFLAGS) -o $@ $^

proxy: proxy.cpp
	$(CC) $(CFLAGS) -o $@ $^

# client: client.cpp
# 	$(CC) $(CFLAGS) -o $@ $^

clean:
	-rm -f *.o *~ *core* proxy #server client