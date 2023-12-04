CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic

all: hello

hello: hello.o
	$(CC) -o hello hello.o

hello.o: hello.c 
	$(CC) $(CFLAGS) hello.c -c -o hello.o

clean:
	rm *.o $(hello) hello

