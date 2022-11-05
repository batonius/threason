CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c2x -march=native -D_POSIX_C_SOURCE=200809 -Iinclude -flto
BUILD-DIR=build
SRC-DIR=src
TEST-DIR=tests
LDFLAGS=-flto
SRCS=$(notdir $(wildcard $(SRC-DIR)/*.c))
OBJS=$(patsubst %.c,%.o,$(SRCS))
TEST-SRCS=$(notdir $(wildcard $(TEST-DIR)/*.c))
TEST-BINS=$(patsubst %.c,%,$(TEST-SRCS))

ifdef GCC 
	CC=gcc
endif

ifdef DEBUG
	CFLAGS+= -g -O0
	LDFLAGS+= -g
else ifdef DEBUGO3
	CFLAGS+= -g -O3
	LDFLATS+= -g
else
    CFLAGS+= -O3
	LDFLAGS+= -s
endif

all: tests

$(BUILD-DIR):
	mkdir $(BUILD-DIR)

$(BUILD-DIR)/%.o: $(SRC-DIR)/%.c $(BUILD-DIR)
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD-DIR)/%: $(TEST-DIR)/%.c $(addprefix $(BUILD-DIR)/, $(OBJS))
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

tests: $(BUILD-DIR) $(addprefix $(BUILD-DIR)/, $(TEST-BINS))
	
clean:
	rm -rf $(BUILD-DIR)
