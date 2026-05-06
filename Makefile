CC = gcc
CFLAGS = -Wall -Isrc -g
LIBS = -lm -lssl -lcrypto -lpthread -ljson-c -lcurl

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/cex

SRCS := $(shell find $(SRC_DIR) -name '*.c' ! -name 'onchain_market.c')
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

bpf:
	clang -target bpf -O2 -c src/market/onchain_market.c -o build/onchain_market.o

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean bpf
