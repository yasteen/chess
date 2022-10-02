CC=gcc
CFLAGS=-Wall -g
LDFLAGS=

SOURCE_DIR=src

TEST_INIT := init_test
TEST_INIT_SRC := test.c

CHESS_SRC := ../src/chess.c
CHESS_OBJ := chess.o
TARGETS = $(TEST_INIT)

# --- BUILD RULES --- #

# Build all
all: $(TARGETS)

# chess file
$(CHESS_OBJ): $(CHESS_SRC)
	$(CC) $(CFLAGS) $^ -c -o $@

# Build executables
$(TEST_INIT): $(TEST_INIT_SRC) $(CHESS_OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(TARGETS)
	rm -f *.o

run:
	./test

.PHONY: clean run all
