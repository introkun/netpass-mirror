# tests/Makefile — build & run host-native logger tests

CC = gcc
CFLAGS = -Wall -Wextra -D__HOST_TEST__ -I../include
SRC = logger_test.c ../source/logger.c
OUTDIR = ../out
OUT = $(OUTDIR)/logger_test
LOG_DIR = log
LOG_FILE = $(LOG_DIR)/test_log.txt

.PHONY: all clean

all: $(OUT)
	@mkdir -p $(LOG_DIR)
	@./$(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(OUT) $(LOG_FILE)
	rm -rf $(LOG_DIR)
