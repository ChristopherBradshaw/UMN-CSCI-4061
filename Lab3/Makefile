all: lib test

lib: mini_filesystem.c
	gcc -c mini_filesystem.c
	ar -cvq libfstest.a mini_filesystem.o
	rm mini_filesystem.o

test: mini_filesystem.c
	gcc -o mini_filesystem test.c libfstest.a

clean:
	rm mini_filesystem
	rm libfstest.a
