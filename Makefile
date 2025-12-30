all: libreader.so main

libreader.so: reader.c jit-reader.h
	$(CC) -shared -o libreader.so reader.c

main: main.c
	$(CC) -ggdb -o main main.c

clean:
	rm -f libreader.so main

.PHONY: all clean
