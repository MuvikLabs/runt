CFLAGS = -g -Wall -ansi -pedantic -fPIC

LDFLAGS = -ldl

OBJ = runt.o basic.o

default: irunt

playground: playground.c $(OBJ) plugin.so
	$(CC) $(LDFLAGS) $(CFLAGS) playground.c $(OBJ) -o $@

irunt: irunt.c $(OBJ)
	$(CC) $(LDFLAGS) $(CFLAGS) irunt.c $(OBJ) -o $@

plugin.so: plugin.c $(OBJ)
	$(CC) plugin.c $(OBJ) -shared -fPIC -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf playground runt.o plugin.so irunt
	rm -rf $(OBJ)
