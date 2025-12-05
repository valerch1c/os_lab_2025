# Простой Makefile для лабы
all: client server

client: client.c factorial.c factorial.h
	gcc -Wall -Wextra -pthread -o client client.c factorial.c

server: server.c factorial.c factorial.h
	gcc -Wall -Wextra -pthread -o server server.c factorial.c

clean:
	rm -f client server

.PHONY: all clean