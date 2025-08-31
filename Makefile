# Makefile for the 'nsrun' project

CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -D_GNU_SOURCE
LDFLAGS = -lpthread

# Source files are in src/ directory
SRCDIR = src
SRC = $(SRCDIR)/main.c $(SRCDIR)/container.c $(SRCDIR)/namespace.c $(SRCDIR)/cgroups.c $(SRCDIR)/network.c
OBJ = $(SRC:.c=.o)
EXEC = nsrun

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

install: $(EXEC)
	sudo cp $(EXEC) /usr/local/bin/

.PHONY: all clean install
