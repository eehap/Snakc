CC = gcc
CFLAGS = -lncurses
SOURCES = ./src/*.c

alL: clean snake

snake:
	$(CC) $(SOURCES) $(CFLAGS) -o snake

clean: 
	rm  -f snake
