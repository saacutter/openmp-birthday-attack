CC = gcc
CFLAGS = -pedantic-errors -Wall -Wextra -Wconversion
OPTIMISATIONS = -flto -funroll-loops -march=native -O3

.PHONY: all clean serial

all: break_hash

break_hash: break_hash.c toy_hash.o
	$(CC) $(CFLAGS) $(OPTIMISATIONS) -fopenmp break_hash.c toy_hash.o -o break_hash

serial: toy_hash.o
	$(CC) $(CFLAGS) $(OPTIMISATIONS) break_hash_serial.c toy_hash.o -o break_hash_serial

toy_hash: toy_hash.c
	$(CC) -c toy_hash.c -o toy_hash.o

clean:
	rm -f toy_hash.o break_hash break_hash_serial