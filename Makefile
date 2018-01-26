CC=gcc
CFLAGS=-Wall -Wextra -g -std=c11

.PHONY:run
.PHONY:clean

buzz: buzz.c
	$(CC) buzz.c -o buzz $(CFLAGS)

clean:
	rm buzz

run: buzz
	#clear
	./buzz
