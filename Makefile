CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -g
OBJS=src/readiso.o src/commands.o
BIN=my_read_iso

all: $(BIN)

$(BIN): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN)

.PHONY: test
test: $(BIN)
	tests/test.sh ./$(BIN) example.iso

clean:
	$(RM) $(OBJS) $(BIN)
