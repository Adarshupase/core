# Makefile for C Parser

# Compiler and flags
CC = gcc
CFLAGS = -I./tree-sitter/lib/include -Wall -Wextra -g
LDFLAGS = -L./tree-sitter -ltree-sitter

# Directories
TREE_SITTER_DIR = ./tree-sitter
C_PARSER_DIR = ./tree-sitter-c
SRC_DIR = ./src

# Source files
SRCS = $(SRC_DIR)/core.c $(SRC_DIR)/main.c $(SRC_DIR)/utils.c $(SRC_DIR)/hash_table.c $(C_PARSER_DIR)/src/parser.c
OBJS = $(SRCS:.c=.o)

# Libraries (using direct path - no -l needed here since we're passing file directly)
LIBS = $(TREE_SITTER_DIR)/libtree-sitter.a

# Target executable
TARGET = c_parser

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build tree-sitter library if needed
$(TREE_SITTER_DIR)/libtree-sitter.a:
	$(MAKE) -C $(TREE_SITTER_DIR) libtree-sitter.a

# Generate C parser if needed
$(C_PARSER_DIR)/src/parser.c:
	cd $(C_PARSER_DIR) && tree-sitter generate

# Clean up
clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(TARGET)

# Run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean distclean run