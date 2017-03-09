all: lib test

lib: mini_filesystem.c mini_filesystem.h
	gcc -c mini_filesystem.c
	ar -cvq libfstest.a mini_filesystem.o

test: libfstest.a
	gcc -o mini_filesystem test.c libfstest.a -lm

clean:
	rm mini_filesystem
	rm libfstest.a
