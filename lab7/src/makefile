# Makefile для TCP/UDP клиентов и серверов

CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Исполняемые файлы
ALL = tcpclient tcpserver udpclient udpserver

# По умолчанию собираем всё
all: $(ALL)

# TCP клиент
tcpclient: tcpclient.c
	$(CC) $(CFLAGS) -o $@ $<

# TCP сервер  
tcpserver: tcpserver.c
	$(CC) $(CFLAGS) -o $@ $<

# UDP клиент
udpclient: udpclient.c
	$(CC) $(CFLAGS) -o $@ $<

# UDP сервер
udpserver: udpserver.c
	$(CC) $(CFLAGS) -o $@ $<

# Очистка
clean:
	rm -f $(ALL)

# Тест TCP
test-tcp: tcpclient tcpserver
	@echo "=== TCP TEST ==="
	@./tcpserver 10050 100 &
	@sleep 1
	@echo "Hello TCP!" | ./tcpclient 127.0.0.1 10050 100
	@pkill tcpserver 2>/dev/null || true

# Тест UDP
test-udp: udpclient udpserver
	@echo "=== UDP TEST ==="
	@./udpserver 20001 1024 &
	@sleep 1
	@echo "Hello UDP!" | ./udpclient 127.0.0.1 20001 1024
	@pkill udpserver 2>/dev/null || true

# Все тесты
test: test-tcp test-udp

# Помощь
help:
	@echo "Команды:"
	@echo "  make all      - собрать всё"
	@echo "  make clean    - очистить"
	@echo "  make test     - запустить тесты"
	@echo ""
	@echo "Запуск:"
	@echo "  ./tcpserver <PORT> <BUFSIZE>"
	@echo "  ./tcpclient <IP> <PORT> <BUFSIZE>"
	@echo "  ./udpserver <PORT> <BUFSIZE>"
	@echo "  ./udpclient <IP> <PORT> <BUFSIZE>"

.PHONY: all clean test test-tcp test-udp help