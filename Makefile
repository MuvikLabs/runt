.PHONY: default install clean

CFLAGS = -g -Wall -ansi -pedantic -fPIC -Ips -I.

SPORTH_LIBS = -lsporth

LDFLAGS = -ldl

OBJ = runt.o basic.o

default: irunt librunt.a ps/runt.so

playground: playground.c $(OBJ) plugin.so
	$(CC) $(LDFLAGS) $(CFLAGS) playground.c $(OBJ) -o $@

irunt: irunt.c $(OBJ)
	$(CC) $(LDFLAGS) $(CFLAGS) irunt.c $(OBJ) -o $@

librunt.a: $(OBJ)
	ar rcs $@ $(OBJ)

plugin.so: plugin.c 
	$(CC) plugin.c -shared -fPIC -o $@ librunt.a

ps/runt.so: ps/runt.c 
	$(CC) $(CFLAGS) ps/runt.c -shared -fPIC -o $@ $(OBJ) $(LDFLAGS) $(SPORTH_LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

install: irunt librunt.a ps/runt.so
	mkdir -p /usr/local/share/sporth/polysporth
	install irunt /usr/local/bin
	install librunt.a /usr/local/lib
	install runt.h /usr/local/include
	install ps/runt.so /usr/local/share/sporth/polysporth

clean:
	rm -rf playground runt.o plugin.so irunt librunt.a ps/runt.so
	rm -rf $(OBJ)
