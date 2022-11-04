CC=clang
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c2x -march=native -D_POSIX_C_SOURCE=200809 -Iinclude
BUILD-DIR=build
SRC-DIR=src
TEST-DIR=tests
LDFLAGS=-flto=full
SRCS=$(notdir $(wildcard $(SRC-DIR)/*.c))
OBJS=$(patsubst %.c,%.o,$(SRCS))
TEST-SRCS=$(notdir $(wildcard $(TEST-DIR)/*.c))
TEST-OBJS=$(patsubst %.c,%.o,$(TEST-SRCS))
TEST-BINS=$(patsubst %.c,%,$(TEST-SRCS))

ifdef DEBUG
	CFLAGS+= -g -O0
	LDFLAGS+= -g
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
	$(CC) $(CFLAGS) $^ -o $@

tests: $(BUILD-DIR) $(addprefix $(BUILD-DIR)/, $(TEST-BINS))
	
clean:
	rm -rf $(BUILD-DIR)
	rm -f httpd
