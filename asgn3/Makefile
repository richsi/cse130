EXECBIN  = queue-test

SOURCES  = $(wildcard *.c)
OBJECTS  = $(SOURCES:%.c=%.o)

CC       = clang
CFLAGS   = -Wall -Werror -Wextra -Wpedantic -Wstrict-prototypes
LFLAGS   = -lpthread

.PHONY: all clean

all: queue.o

queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXECBIN): $(OBJECTS)
	$(CC) $(LFLAGS) -o $@ $^

%.o : %.c
	$(CC) $(CFLAGS) -c $<

format:
	clang-format -i -style=file $(SOURCES)
clean:
	rm -f $(EXECBIN) $(OBJECTS)
