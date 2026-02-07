#ifndef __CORE_H
#define __CORE_H
#include <tree_sitter/api.h>

#include "utils.h"
typedef uint32_t u32;
const TSLanguage *tree_sitter_c(void);
typedef struct {
    TSTree *tree;
    const char *source_code;
    TSParser *parser;
} TSTreeInfo;

typedef enum {
    CHANGE_STRUCT_FIELD,
    CHANGE_STRUCT_NAME,
    COMMAND_NOT_FOUND,
    ARGUMENT_MISMATCH,
    COMMAND_FOUND
} COMMAND_TYPE;

typedef struct {
    char *command;
    char **args;
    int number_of_arguments;

} Core_Command;
typedef struct EditSpan {
    u32 start;
    u32 end;
} EditSpan;

void traverse(TSNode root_node,int nest);
void print_tree(TSNode root_node);
void print_tree_with_cursor(TSTreeCursor *tree_cursor);
void traverse_and_debug(TSNode node, int nest, const char *source );
void debug_tree(TSNode root_node, const char *source);
void change_field_in_struct(char **modified_ptr, TSNode node, const char *from,
                             const char *to, TSInputEdit *edit);
void change_struct_field(const char *struct_name,const char *from,const char *to,TSTreeInfo *info);
void change_struct_name(const char *struct_name,const char *to,TSTreeInfo *info);
void change_name_in_struct_declaration(
    char **modified_ptr,
    TSNode struct_node,
    const char *to,
    TSTreeInfo *info);
void change_struct_name_in_program(const char *struct_name,const char *to, TSTreeInfo *info,char *modified_source);
void change_struct_field_in_program(char **modified_source_ptr, const char *struct_name, const char *from,
                                    const char *to, TSInputEdit *edit, TSNode root_node);
void ts_cleanup(TSTreeInfo *info) ;
void print_commands(Core_Command *commands, int total) ;

TSNode find_child_node_of_type(TSNode node, const char *type);
TSNode find_struct_with_name(const char *source,const char *struct_name, TSNode root_node);
TSTree *get_new_tree(TSTreeInfo *info, char *modified_source);
void execute_commands(Core_Command *commands, int total_commands,TSTreeInfo *info);
void free_commands(Core_Command *commands,int total_commands);

COMMAND_TYPE search_for_command(const char *command, int number_of_arguments);
Core_Command *parse_commands(const char *filename, int *total_commands);

#endif // __CORE_H 