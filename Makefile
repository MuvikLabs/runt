CFLAGS = -g -Wall -ansi -pedantic

playground: playground.c runt.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf playground runt.o
