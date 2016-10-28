CFLAGS = -g -Wall -ansi -pedantic

OBJ = runt.o basic.o

default: irunt

playground: playground.c runt.o
	$(CC) $(CFLAGS) $^ -o $@

irunt: irunt.c $(OBJ)
	$(CC) $(CFLAGS) irunt.c $(OBJ) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf playground runt.o
