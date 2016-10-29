CFLAGS = -g -Wall -ansi -pedantic -fPIC

LDFLAGS = -ldl

OBJ = runt.o basic.o

default: irunt librunt.a

playground: playground.c $(OBJ) plugin.so
	$(CC) $(LDFLAGS) $(CFLAGS) playground.c $(OBJ) -o $@

irunt: irunt.c $(OBJ)
	$(CC) $(LDFLAGS) $(CFLAGS) irunt.c $(OBJ) -o $@

librunt.a: $(OBJ)
	ar rcs $@ $(OBJ)

plugin.so: plugin.c 
	$(CC) plugin.c -shared -fPIC -o $@ librunt.a

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@

clean:
	rm -rf playground runt.o plugin.so irunt librunt.a
	rm -rf $(OBJ)
