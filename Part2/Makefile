all: tcp
	
tcp: server client

server: server.o
	gcc -g -o server server.c

client: client.o
	gcc -g -o client client.o -lncurses

directory: directory.o
	gcc -g -o directory directory.c

clean:
	rm *.o \
	    server client directory
