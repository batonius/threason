CC=clang
AR=ar
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c2x -march=native \
	 -D_POSIX_C_SOURCE=200809 -D_GNU_SOURCE -Iinclude -flto -Ilib
LDFLAGS=
BUILD-DIR=build
SRC-DIR=lib
BIN-DIR=bin
TEST-DIR=lib/tests
LIB-AR=$(BUILD-DIR)/libthreason.a
SRCS=$(notdir $(wildcard $(SRC-DIR)/*.c))
OBJS=$(addprefix $(BUILD-DIR)/, $(patsubst %.c,%.o,$(SRCS)))
TEST-SRCS=$(notdir $(wildcard $(TEST-DIR)/*.c))
TEST-BINS=$(patsubst %.c,%,$(TEST-SRCS))

ifdef GCC 
	CC=gcc
endif

ifdef DEBUG
	CFLAGS+= -g -O0
else ifdef DEBUGO3
	CFLAGS+= -g -O3
else
    CFLAGS+= -O3
	LDFLAGS+= -s
endif

ifdef ASAN
	CFLAGS+= -fsanitize=address
	LDFLAGS+= -fsanitize=address
endif

ifdef UBSAN
	CFLAGS+= -fsanitize=undefined
	LDFLAGS+= -fsanitize=undefined
endif

.SUFFIXES:

.PHONY: all clean tests

all: tests

$(BUILD-DIR):
	mkdir $(BUILD-DIR)

$(BUILD-DIR)/%.o: $(SRC-DIR)/%.c | $(BUILD-DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB-AR): $(OBJS)
	$(AR) cr $@ $^

$(BUILD-DIR)/%: $(BIN-DIR)/%.c $(LIB-AR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD-DIR)/%: $(TEST-DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@
 
tests: $(addprefix $(BUILD-DIR)/, $(TEST-BINS)) | $(BUILD_DIR)
	
clean:
	rm -rf $(BUILD-DIR)
