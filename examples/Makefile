default: all

all: simple lowlevel api last_word

simple: simple.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

lowlevel: lowlevel.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

api: api.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

last_word: last_word.c
	$(CC) $< -o $@ -I../ ../librunt.a -ldl

clean:
	$(RM) simple
	$(RM) lowlevel
	$(RM) api
	$(RM) last_word
