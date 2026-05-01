CC      = gcc
CFLAGS  = -I../tree-sitter/lib/include -Wall -Wextra -g
LDFLAGS = -L../tree-sitter -ltree-sitter

TREE_SITTER_DIR = ../tree-sitter
C_PARSER_DIR 	= ../tree-sitter-c
CORE_SRC_DIR 	= ../src
SRCS  = main.c  $(CORE_SRC_DIR)/core.c $(CORE_SRC_DIR)/utils.c $(CORE_SRC_DIR)/hash_table.c $(CORE_SRC_DIR)/cmd.c $(C_PARSER_DIR)/src/parser.c 

OBJS  = $(SRCS:.c=.o)

LIBS  = $(TREE_SITTER_DIR)/libtree-sitter.a
TARGET = c_test

all : $(TARGET)

$(TARGET): $(OBJS) $(LIBS)
	$(CC) $(OBJS) $(LIBS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

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
