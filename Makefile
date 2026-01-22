# Makefile for System Programming Assignment 4
CC = gcc
CFLAGS = -Wall -g -pthread

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

clean:
	rm -f server client