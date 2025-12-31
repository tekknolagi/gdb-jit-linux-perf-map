all: libreader.so main

libreader.so: reader.c jit-reader.h
	$(CC) -std=c99 -shared -o libreader.so reader.c

main: main.c
	$(CC) -std=c99 -ggdb -o main main.c

clean:
	rm -f libreader.so main

.PHONY: all clean
