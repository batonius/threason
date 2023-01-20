CC=clang
CXX=clang++
AR=ar
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c11 -march=native -Iinclude -flto 
CXXFLAGS=-std=c++17 -march=native -flto
BIN-CFLAGS=$(CFLAGS) -D_POSIX_C_SOURCE=199309L
LDFLAGS=
BUILD-DIR=build
SRC-DIR=lib
BIN-DIR=bin
SIMDJSON-DIR=third_party/simdjson
SIMDJSON-SRC=$(SIMDJSON-DIR)/simdjson.cpp
SIMDJSON-OBJ=$(BUILD-DIR)/simdjson.o
YYJSON-DIR=third_party/yyjson
YYJSON-SRC=$(YYJSON-DIR)/yyjson.c
YYJSON-OBJ=$(BUILD-DIR)/yyjson.o
BENCH-DIR=benchmarks
TWEETS-SRC=$(BENCH-DIR)/tweets/tweets.cpp
TWEETS-BIN=$(BUILD-DIR)/tweets
JSONS-DIR=jsons
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
	CXX=g++
endif

ifdef DEBUG
	CFLAGS+= -g -O0
	CXXFLAGS+= -g -O0
else ifdef DEBUGO3
	CFLAGS+= -g -O3 -fno-omit-frame-pointer
	CXXFLAGS+= -g -O3 -fno-omit-frame-pointer
else
    CFLAGS+= -O3
	CXXFLAGS+= -O3
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

.PHONY: all clean tests benchmarks run-tests 

all: $(LIB-AR) tests bins

$(BUILD-DIR):
	mkdir $(BUILD-DIR)

$(BUILD-DIR)/%.o: $(SRC-DIR)/%.c | $(BUILD-DIR)
	$(CC) -xc $(CFLAGS) -I$(SRC-DIR) -c $< -o $@

$(LIB-AR): $(OBJS)
	$(AR) cr $@ $^

$(BUILD-DIR)/%: $(BIN-DIR)/%.c $(LIB-AR)
	$(CC) $(BIN-CFLAGS) $(LDFLAGS) $^ -o $@

$(BUILD-DIR)/%: $(TEST-DIR)/%.c $(OBJS)
	$(CC) $(CFLAGS) -I$(SRC-DIR) $(LDFLAGS) $^ -o $@

$(SIMDJSON-OBJ): $(SIMDJSON-SRC) | $(BUILD-DIR)
	$(CXX) $(CXXFLAGS) -I$(SIMDJSON-DIR) -c $< -o $@

$(YYJSON-OBJ): $(YYJSON-SRC) | $(BUILD-DIR)
	$(CC) $(CFLAGS) -I$(YYJSON-DIR) -c $< -o $@

$(TWEETS-BIN): $(TWEETS-SRC) $(SIMDJSON-OBJ) $(YYJSON-OBJ) $(LIB-AR) | $(BUILD-DIR)
	$(CXX) $(CXXFLAGS) -I$(SIMDJSON-DIR) -I$(YYJSON-DIR) -Iinclude $(LDFLAGS) $^ -o $@
 
tests: $(addprefix $(BUILD-DIR)/, $(TEST-BINS))

bins: $(addprefix $(BUILD-DIR)/, $(BIN-BINS)) 

benchmarks: $(TWEETS-BIN)

$(DOCS-DIR):
	doxygen
	
clean:
	rm -rf $(BUILD-DIR)
	rm -rf $(DOCS-DIR)

run-tests: clean tests
	$(BUILD-DIR)/tests
