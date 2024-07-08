# the executables will be built in the "build" directory.
# the executables can be run in a terminal/shell
# cmd: ./pathto/executable
# for tests where files have to be passed as arguments
# cmd: ./pathto/executable pathto/file1 pathto/file2

# Directories
SRC_DIR = src
TEST_DIR = test
TEST_SUBDIRS = $(shell find $(TEST_DIR) -type d)
INCLUDE_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS = $(foreach dir, $(TEST_SUBDIRS), $(wildcard $(dir)/*.c))

# Object files
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Target
TEST_TARGET = $(foreach test_src, $(TEST_SRCS), $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%, $(test_src)))

# Compiler
CC = clang

# Compiler flags
CFLAGS = -Wall -Wextra -I$(INCLUDE_DIR) -pthread -g -gdwarf-4

# Default rule
all: $(TEST_TARGET)

# Rule for compiling test source files into test targets
$(BUILD_DIR)/%: $(TEST_DIR)/%.c $(OBJS) | $(BUILD_DIR) 
	$(CC) $(CFLAGS) $(OBJS) $< -o $@

# Rule for compiling source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Create build subsdirectories if they don't exist
$(foreach dir, $(TEST_SUBDIRS), $(shell mkdir -p $(patsubst $(TEST_DIR)/%, $(BUILD_DIR)/%, $(dir))))

# Clean up
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean

.PHONY: pack
pack:
	zip -r submission.zip src/
