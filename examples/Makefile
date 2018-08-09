default: all

all: simple lowlevel api

simple: simple.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

lowlevel: lowlevel.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

api: api.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

clean:
	$(RM) simple
	$(RM) lowlevel
	$(RM) api
