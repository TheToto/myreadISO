CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g
OBJS=src/readiso.o
BIN=myreadiso

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)

clean:
	$(RM) $(OBJS) $(BIN)
