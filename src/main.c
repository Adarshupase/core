
#include <string.h>
#include <stdio.h>
#include "utils.h"
#include "core.h"
typedef uint32_t u32; 

#define VERBOSE 0
#define CURSOR_MODE 0 


void print_all_nodes_for_query(const char *query_string, TSNode node, const char *source) 
{
    TSQueryError error_type; u32 error_offset;

    TSQuery *query = ts_query_new(
        tree_sitter_c(),
        query_string, 
        strlen(query_string),
        &error_offset,
        &error_type
    );

    TSQueryCursor *cursor = ts_query_cursor_new();
    TSQueryMatch match;

    ts_query_cursor_exec(cursor, query,node );
    while(ts_query_cursor_next_match(cursor, &match)){
        for(u32 i = 0; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            debug_tree(node,source);
            printf("__________DEBUG____________\n");
        }
    }

}

int main(int argc, char *argv[]) 
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());
    const char *source_code = "struct Mutton {}; struct Halwa {int time_taken; char *nmae; };";
    const char *query_string = "(field_declaration) @field";
    
    if(argc > 1) {
        source_code = read_entire_file(argv[1]);
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

    TSTreeInfo info = {
        .tree = tree,
        .source_code = source_code,
        .parser = parser
    };

    TSNode root_node = ts_tree_root_node(info.tree);
    TSTreeCursor tree_cursor = ts_tree_cursor_new(root_node);
    if (VERBOSE) {
        if(CURSOR_MODE){
            print_tree_with_cursor(&tree_cursor);
        } else {
            print_tree(root_node);
        }
    }
    
    char *node_string = ts_node_string(root_node);
    printf("%s\n", node_string);
    
    pretty_print_tree(node_string);
    

    // printf("--------------------------------------------------------------------------------------------------------------\n");
    if(VERBOSE){
        printf("Source Code: %s\n", source_code);
        printf("Query String: %s\n", query_string);
    }

    u32 error_offset ;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(
        tree_sitter_c(),
        query_string,
        strlen(query_string),
        &error_offset,
        &error_type
    );
    TSQueryCursor *query_cursor = ts_query_cursor_new();
    TSNode struct_node = find_struct_with_name(info.source_code, "Halwa", root_node);
    
    ts_query_cursor_exec(query_cursor,query,struct_node);
    TSQueryMatch match;
    while(ts_query_cursor_next_match(query_cursor,&match)){
        for(u32 i= 0 ; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            TSNode name_node = find_child_node_of_type(node,"field_identifier");
            if(!ts_node_is_null(name_node)){
                u32 start_byte = ts_node_start_byte(name_node);
                u32 end_byte   = ts_node_end_byte(name_node);
                int slice_size = end_byte - start_byte;
                char *string = strndup(info.source_code + start_byte, slice_size);
                // printf("%s", string);
                free(string);
            }
        }
    }
    
    // printf("----------------------------DEBUG_INFO--------------------------------------------------\n");
    // printf("____________________________BEFORE-CHANGE________________________________________________\n");
    
    // debug_tree(root_node,info.source_code);
    // change_struct_field("Halwa","a","b",&info);
    // change_struct_field("Halwa","x","y",&info);
    // change_struct_field("Halwa","m","n",&info);
    // change_struct_field("Halwa","e","f",&info);
    // change_struct_field("Halwa","s","t",&info);

    if(argc > 1) {
        const char *file_name = argv[1];
        write_modified_string_to_file(file_name,info.source_code);
    }

    root_node = ts_tree_root_node(info.tree);
    // printf("____________________________AFTER-CHANGE________________________________________________\n");
    // debug_tree(root_node, info.source_code);
    
    
    // const char *string_query ="(field_declaration declarator: (field_identifier) @field_name)";
    // print_all_nodes_for_query(string_query, root_node, info.source_code);
    int total_commands;
    
    Core_Command *commands = parse_commands("core.txt",&total_commands);
    
    print_commands(commands,total_commands);
    execute_commands(commands,total_commands,&info);
    change_struct_name("Entity","Component",&info);
    if(argc > 1) {
        const char *file_name = argv[1];
        write_modified_string_to_file(file_name,info.source_code);
    }
    free_commands(commands,total_commands);
    free(node_string);
    ts_query_delete(query);
    ts_tree_cursor_delete(&tree_cursor);
    ts_query_cursor_delete(query_cursor);
    ts_cleanup(&info);

}