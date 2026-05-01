#ifndef __CORE_H
#define __CORE_H
#include "utils.h"
#include <tree_sitter/api.h>


#define TS_NODE_SLICE(node, start, end, size) \
    do {                                     \
        (start) = ts_node_start_byte(node); \
        (end)   = ts_node_end_byte(node);   \
        (size)  = (end) - (start);          \
    } while (0)

typedef enum {
    FUNCTION_DECLARATION,
    FUNCTION_DEFINITION,
    STRUCT_DEFINITION,
    FUNCTION_CALL,
    STRUCT_DECLARATION,
    LOCAL_VARIABLE,
} Node_Type;

typedef enum {
  STRUCT_TYPE,
  ENUM_TYPE,
} Data_Type;

typedef struct {
  TSNode first;
  TSNode second;
} TSNode_Pair;

typedef struct {
    TSQuery *query;
    TSQueryCursor *cursor;
} Query_Context;



typedef uint32_t u32;
const TSLanguage *tree_sitter_c(void);
typedef struct {
    TSTree *tree;
    const char *source_code;
    TSParser *parser;
} TSTreeInfo;

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
void change_function_name(const char *function_name, const char *to, TSTreeInfo *info) ;
void change_variable_in_function(const char *function_name, const char *from, const char *to, TSTreeInfo *info);
void change_name_in_struct_declaration(
    char **modified_ptr,
    TSNode struct_node,
    const char *to,
    TSTreeInfo *info);

void change_struct_name_in_program(const char *struct_name,const char *to, TSTreeInfo *info,char *modified_source);
void change_struct_field_in_program(char **modified_source_ptr, const char *struct_name, const char *from,
                                    const char *to, TSInputEdit *edit, TSNode root_node);
void ts_cleanup(TSTreeInfo *info) ;


TSNode find_child_node_of_type(TSNode node, const char *type);
TSNode_Pair find_struct_with_name(const char *source,const char *struct_name, TSNode root_node);
TSTree *get_new_tree(TSTreeInfo *info, char *modified_source);
Query_Context create_query_context(const char *query_string, TSNode root_node);


#endif // __CORE_H 
