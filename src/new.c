#define _GNU_SOURCE
#include <tree_sitter/api.h> 
#include <string.h>
#include <stdio.h>
#include <assert.h>

typedef uint32_t u32;
#define CURSOR_MODE 0
#define VERBOSE 0


// definitions 
void traverse(TSNode root_node,int nest);
void print_tree(TSNode root_node);
void print_tree_with_cursor(TSTreeCursor *tree_cursor);
TSNode find_struct_with_name(const char *source,const char *struct_name, TSNode root_node);
const TSLanguage *tree_sitter_c(void);
void traverse(TSNode root_node, int nest)
{
    for(u32 i = 0; i < ts_node_child_count(root_node); i++) 
    {
        TSNode child = ts_node_child(root_node,i);
        for(int i = 0; i < nest ; i++) {
            printf(" ");
        }
        printf("%s",ts_node_type(child));
        printf("\n");
        traverse(child,nest + 1);
    }
}

void print_tree(TSNode root_node) 
{
    traverse(root_node, 0);
}
void print_tree_with_cursor(TSTreeCursor *tree_cursor) 
{
     do {
        const char *field = ts_tree_cursor_current_field_name(tree_cursor);
        if(field) {
            printf("%s\n",field);
        }
        if(ts_tree_cursor_goto_first_child(tree_cursor)) {
            print_tree_with_cursor(tree_cursor);
            ts_tree_cursor_goto_parent(tree_cursor);
        }
    } while(ts_tree_cursor_goto_next_sibling(tree_cursor));
}

TSNode find_struct_with_name( const char *source, const char *struct_name, TSNode root_node ) 
{ 
    const char *query_string = "(struct_specifier name: (type_identifier) @name ) @struct"; 
    TSQueryError error_type; u32 error_offset; 
    TSQuery *query = ts_query_new( 
        tree_sitter_c(), 
        query_string,
        strlen(query_string), 
        &error_offset, 
        &error_type 
    );
    TSQueryCursor *cursor = ts_query_cursor_new(); 
    ts_query_cursor_exec(cursor, query, root_node); 
    TSQueryMatch match; TSNode result = {0}; 

    while (ts_query_cursor_next_match(cursor, &match)) { 
        TSNode name_node = {0}; 
        TSNode struct_node = {0}; 
        for (u32 i = 0; i < match.capture_count; i++) { 
             u32 length;
             const char *cap = ts_query_capture_name_for_id( query, match.captures[i].index, &length );
             if (strcmp(cap, "name") == 0) name_node = match.captures[i].node;
             else if (strcmp(cap, "struct") == 0) struct_node = match.captures[i].node; 
        }
        if (!ts_node_is_null(name_node)) 
        { 
            u32 start = ts_node_start_byte(name_node); 
            u32 end = ts_node_end_byte(name_node); 
            if (strncmp(source + start, struct_name, end - start) == 0 && struct_name[end - start] == '\0') 
            { 
                
                result = struct_node; 
                break; 
            } 
        } 
    } 
    ts_query_cursor_delete(cursor); 
    ts_query_delete(query); 
    return result; 
}


int main(int argc, char *argv[]) 
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());
    const char *source_code = "struct Mutton {}; struct Halwa {char *dick;};";
    const char *query_string = "(binary_expression (number_literal)(number_literal)) @bin";

    
    
    if(argc > 1) {
        source_code = argv[1];
    }
    if(argc > 2) {
        query_string = argv[2];
    }

    char *modified_source = malloc(strlen(source_code) + 1);
    strcpy(modified_source,source_code);

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code)
    );
    TSNode root_node = ts_tree_root_node(tree);
    TSTreeCursor tree_cursor = ts_tree_cursor_new(root_node);
    if (VERBOSE) {
        if(CURSOR_MODE){
            print_tree_with_cursor(&tree_cursor);
        } else {
            print_tree(root_node);
        }
    }
    
    char *node_string = ts_node_string(root_node);
    if(VERBOSE){
        printf("%s\n", node_string);
    }

    printf("--------------------------------------------------------------------------------------------------------------\n");
    printf("Source Code: %s\n", source_code);
    printf("Query String: %s\n", query_string);
    
    const TSLanguage *language = tree_sitter_c();



    u32 error_offset ;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(
        language,
        query_string,
        strlen(query_string),
        &error_offset,
        &error_type
    );
    TSQueryCursor *query_cursor = ts_query_cursor_new();

    ts_query_cursor_exec(query_cursor,query,root_node);
    TSQueryMatch match;
    while(ts_query_cursor_next_match(query_cursor,&match)){
        for(u32 i= 0 ; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            char *node_string = ts_node_string(node);
            printf("matched node: %s\n",node_string);
            free(node_string);
        }
    }
    const char *name = "Halwa";
    TSNode node = find_struct_with_name(source_code,name,root_node);
    if(ts_node_is_null(node)){
        printf("THis shit is shit");
    } else {
        char *good = ts_node_string(node);
        printf("ENTER THE SANDMAN\n");
        printf("%s\n",good);
        free(good);
    }
    free(node_string);
    ts_query_delete(query);
    ts_query_cursor_delete(query_cursor);
    ts_tree_delete(tree);
    ts_parser_delete(parser);

}