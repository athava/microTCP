CC=gcc

all:make

make: udp_client.c udp_server.c
	$(CC) udp_client.c -o client.out
	$(CC) udp_server.c -o server.out

client:
	$(CC) udp_client.c -o client.out

server:
	$(CC) udp_server.c -o server.out
clean:
	rm server.out client.out
