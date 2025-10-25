CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pthread
LDFLAGS = -pthread


all: parallel_min_max process_memory parallel_sum

# Сборка parallel_min_max (как было)
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Сборка process_memory (добавил)
process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

# Правила для объектных файлов (как было)
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

parallel_sum: parallel_sum.o utils.o sum_lib.o
	$(CC) -o $@ $^ $(LDFLAGS)

parallel_sum.o: parallel_sum.c utils.h sum_lib.h
	$(CC) $(CFLAGS) -c parallel_sum.c

sum_lib.o: sum_lib.c sum_lib.h
	$(CC) $(CFLAGS) -c sum_lib.c


clean :
	rm -f utils.o find_min_max.o sequential_min_max parallel_min_max process_memory

.PHONY: all clean