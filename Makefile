.PHONY: default install clean

CFLAGS = -g -Wall -ansi -pedantic -fPIC -Ips -I.

SPORTH_LIBS = -lsporth -lsoundpipe -lsndfile -lm

LDFLAGS = -ldl -L/usr/local/lib

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

install: default
	install irunt /usr/local/bin
	install librunt.a /usr/local/lib
	install runt.h /usr/local/include

clean:
	rm -rf playground runt.o plugin.so irunt librunt.a 
	rm -rf $(OBJ)
