CC = gcc
CFLAGS = -Wall -Wextra -Isrc -g
LIBS = -lm -lssl -lcrypto -lpthread -lwebsockets -ljson-c

SRC_DIR = src
BUILD_DIR = build
TARGET = $(BUILD_DIR)/cex

# Find all .c files in src and its subdirectories
SRCS := $(shell find $(SRC_DIR) -name '*.c')
OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Rule to link the program
$(TARGET): $(OBJS)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

# Rule to compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
