BINARY_NAME = mdto

CC = gcc
CFLAGS = -g -Wall -Wextra --pedantic-errors

INSTALL = install -C
BINDIR = /bin/
SRCS = main.c md.c html.c
OBJS = $(SRCS:.c=.o)

all: build

build: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BINARY_NAME)

install: build
	${INSTALL} ./${BINARY_NAME} ${BINDIR}

test:
	./${BINARY_NAME} tests/test1.md

clean:
	rm -f *.o $(BINARY_NAME)
