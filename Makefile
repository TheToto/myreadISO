CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g -I.
OBJS=src/readiso.o src/commands.o
BIN=myreadiso

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)

.PHONY: test
test:
	test/test.sh ./$(BIN)

clean:
	$(RM) $(OBJS) $(BIN)
