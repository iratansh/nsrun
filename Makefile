# Makefile for the 'nsrun' project

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11
LDFLAGS = -lpthread

SRC = main.c container.c namespace.c cgroups.c network.c
OBJ = $(SRC:.c=.o)
EXEC = nsrun

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)
