default: all

all: simple lowlevel

simple: simple.c
	$(CC) $< -o $@ -I../ -L../ -lrunt -ldl

lowlevel: lowlevel.c
	$(CC) $< -o $@ -I../ -L../ -lrunt -ldl

clean:
	rm -rf simple
	rm -rf lowlevel 
