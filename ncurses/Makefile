CC = gcc
CFLAGS = -lpthread
LDFLAGS = -static


all: server client

server:
	$(CC) -o server server.c $(CFLAGS) $(LDFLAGS)

client:
	$(CC) -o client client.c $(CFLAGS) -lncursesw $(LDFLAGS)


clean:
	rm server \
	    rm client
