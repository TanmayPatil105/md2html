BINARY_NAME = mdto

CC = gcc
CFLAGS = -g -Wall -Wextra --pedantic-errors

SRCS = main.c md.c html.c
OBJS = $(SRCS:.c=.o)

all: build

build: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BINARY_NAME)

clean:
	rm -f *.o $(BINARY_NAME)
