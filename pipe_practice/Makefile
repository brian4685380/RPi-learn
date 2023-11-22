CC = gcc
CFLAGS = -g -Wall

all: main client encrypt

main: main.c
	$(CC) $(CFLAGS) main.c -o main

client: client.c
	$(CC) $(CFLAGS) client.c -o client

encrypt: encrypt.c
	$(CC) $(CFLAGS) encrypt.c -o encrypt

clean:
	rm -f main client encrypt