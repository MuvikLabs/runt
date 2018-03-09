.PHONY: default install clean plugin

CFLAGS += -g -Wall -std=c89 -pedantic -I. -fPIC

SPORTH_LIBS = -lsporth -lsoundpipe -lsndfile -lm

LDFLAGS += -ldl -L/usr/local/lib

OBJ += runt.o basic.o irunt.o

CONFIG?=config.mk

include $(CONFIG)

default: irunt librunt.a 

ifdef ALIGNED_MALLOC
CFLAGS += -DALIGNED_MALLOC
endif

ifdef RUNT_PLUGINS
CFLAGS +=-DRUNT_PLUGINS
endif

playground: playground.c $(OBJ) plugin.so
	$(CC) $(CFLAGS) playground.c $(OBJ) -o $@ $(LDFLAGS)

irunt: main.c $(OBJ)
	$(CC) $(CFLAGS) main.c $(OBJ) -o $@ $(LDFLAGS)

librunt.a: $(OBJ)
	ar rcs $@ $(OBJ)

plugin: plugin.so

plugin.so: plugin.c 
	$(CC) plugin.c -shared -fPIC -o $@ librunt.a

%.o: %.c
	$(CC) $(CFLAGS) -c $^ -o $@ 

install: default
	mkdir -p $(HOME)/.runt/lib
	mkdir -p $(HOME)/.runt/bin
	mkdir -p $(HOME)/.runt/include
	install irunt $(HOME)/.runt/bin
	install librunt.a $(HOME)/.runt/lib
	install runt.h $(HOME)/.runt/include

clean:
	rm -rf playground runt.o plugin.so irunt librunt.a 
	rm -rf $(OBJ)
