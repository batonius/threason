CC=clang
AR=ar
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c2x -march=native \
       -D_GNU_SOURCE -Iinclude -flto 
LDFLAGS=
BUILD-DIR=build
SRC-DIR=lib
BIN-DIR=bin
TEST-DIR=lib/tests
DOCS-DIR=docs
LIB-AR=$(BUILD-DIR)/libthreason.a
SRCS=$(notdir $(wildcard $(SRC-DIR)/*.c))
OBJS=$(addprefix $(BUILD-DIR)/, $(patsubst %.c,%.o,$(SRCS)))
TEST-SRCS=$(notdir $(wildcard $(TEST-DIR)/*.c))
TEST-BINS=$(patsubst %.c,%,$(TEST-SRCS))
BIN-SRCS=$(notdir $(wildcard $(BIN-DIR)/*.c))
BIN-BINS=$(patsubst %.c,%,$(BIN-SRCS))

ifdef GCC 
	CC=gcc
endif

ifdef DEBUG
	CFLAGS+= -g -O0
else ifdef DEBUGO3
	CFLAGS+= -g -O3 -fno-omit-frame-pointer
else
    CFLAGS+= -O3
	LDFLAGS+= -s
endif

ifdef ASAN
	CFLAGS+= -fsanitize=address
	LDFLAGS+= -fsanitize=address
endif

ifdef TSAN
	CFLAGS+= -fsanitize=thread
	LDFLAGS+= -fsanitize=thread
endif

ifdef UBSAN
	CFLAGS+= -fsanitize=undefined
	LDFLAGS+= -fsanitize=undefined
endif

ifdef METRICS
	CFLAGS+= -DMETRICS
endif

.SUFFIXES:

.PHONY: all clean tests run-tests

all: $(LIB-AR) tests bins

$(BUILD-DIR):
	mkdir $(BUILD-DIR)

$(BUILD-DIR)/%.o: $(SRC-DIR)/%.c | $(BUILD-DIR)
	$(CC) $(CFLAGS) -I$(SRC-DIR) -c $< -o $@

$(LIB-AR): $(OBJS)
	$(AR) cr $@ $^

$(BUILD-DIR)/%: $(BIN-DIR)/%.c $(LIB-AR)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD-DIR)/%: $(TEST-DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) -I$(SRC-DIR) $(LDFLAGS) $^ -o $@
 
tests: $(addprefix $(BUILD-DIR)/, $(TEST-BINS))

bins: $(addprefix $(BUILD-DIR)/, $(BIN-BINS)) 

$(DOCS-DIR):
	doxygen
	
clean:
	rm -rf $(BUILD-DIR)
	rm -rf $(DOCS-DIR)

run-tests: clean tests
	$(BUILD-DIR)/tests
