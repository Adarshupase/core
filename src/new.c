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
void debug_tree(TSNode root_node);
void change_field_in_struct(char *modified, TSNode node, const char *from,
                             const char *to,TSInputEdit *edit);
TSTree *change_struct_field(const char *struct_name,const char *from,const char *to,TSTree *tree, 
                            TSNode root_node,const char *source);
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


// @UNIMPLEMENTED() this should print tree nodes with their names so i can verify if the tree has indeed changed
void debug_tree(TSNode root_node) 
{
    
}
void change_field_in_struct(char *modified,
                            TSNode node,
                            const char *from,
                            const char *to,
                            TSInputEdit *edit)
{
    TSNode fields;
    const char *intended_child = "field_declaration_list";
    for(u32 i = 0; i < ts_node_child_count(node); i++){
        TSNode child = ts_node_child(node,i);
        if(strcmp(intended_child,ts_node_type(child))==0){
            fields = child;
            break;
        }
    }
    printf("ENTER THE CHILD PRINTER");
    for(u32 i = 0; i < ts_node_child_count(fields); i++) {
        char *string = ts_node_string(ts_node_child(fields,i));
        printf("%s\n", string);
        free(string);
    }


    // char *string = ts_node_string(node);
    // printf("ENTER THE CHANGE FUNCTION\n");
    // printf("%s\n",string);

    const char *query_string = "(field_declaration type: (primitive_type) declarator: (field_identifier) @name)";
    // @TODO(make this as a macro) my code is already breaking i don't want to add more uncertainity
    TSQueryError error_type; u32 error_offset;

    TSQuery *query = ts_query_new(
        tree_sitter_c(), 
        query_string, 
        strlen(query_string),
        &error_offset,
        &error_type
    );

    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor, query, fields);
    
    TSQueryMatch match;
    TSNode name_node = {0};
    while(ts_query_cursor_next_match(cursor, &match)){
        for(u32 i = 0; i < match.capture_count; i++) {
            u32 length;
            const char *captured_name = ts_query_capture_name_for_id(query,match.captures[i].index,&length);
            if(strcmp(captured_name,"name")==0){
                name_node = match.captures[i].node;
            }
        }
        if(!ts_node_is_null(name_node)){
            u32 start_byte = ts_node_start_byte(name_node);
            u32 end_byte   = ts_node_end_byte(name_node);
            u32 slice_size = end_byte - start_byte;

            if (strncmp(modified + start_byte, from, slice_size) == 0 && from[slice_size] == '\0') 
            { 
                size_t new_slice_size = strlen(to);
                if(new_slice_size != slice_size) {
                    memmove(
                        modified + start_byte + new_slice_size,
                        modified + end_byte, 
                        strlen(modified) - end_byte + 1
                    );
                }
                memcpy(modified + start_byte, to, new_slice_size);
                edit->start_byte = start_byte;
                edit->old_end_byte = end_byte;
                edit->new_end_byte = end_byte + new_slice_size - slice_size;
                edit->start_point = ts_node_start_point(name_node);
                edit->old_end_point = ts_node_end_point(name_node);
                edit->new_end_point = (TSPoint){
                    .row = ts_node_end_point(name_node).row,
                    .column = ts_node_end_point(name_node).column + new_slice_size - slice_size
                };
                break;
            } 
        }
    }
    ts_query_cursor_delete(cursor); 
    ts_query_delete(query); 

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
    TSQueryMatch match;
    TSNode result = {0}; 

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
// Source - https://stackoverflow.com/a
// Posted by user529758, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-27, License - CC BY-SA 4.0

TSTree *change_struct_field(const char *struct_name,
                    const char *from,
                    const char *to,
                    TSTree *tree,
                    TSNode root_node,
                    const char *source)
{
    TSNode struct_node = find_struct_with_name(source,struct_name,root_node);
    if(ts_node_is_null(struct_node)) {
        return tree;
    }
    char *modified_source = malloc(strlen(source) + 1);
    strcpy(modified_source,source);
    TSInputEdit edit = {0};
    change_field_in_struct(modified_source, struct_node, from, to, &edit);
    ts_tree_edit(tree, &edit);
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());
    
    TSTree *new_tree = ts_parser_parse_string(
        parser, 
        tree,
        modified_source,
        strlen(modified_source)
    );
    ts_tree_delete(tree);
    ts_parser_delete(parser);
    free(modified_source);
    return new_tree;
}
char *read_entire_file(const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return string;
}


int main(int argc, char *argv[]) 
{
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());
    const char *source_code = "struct Mutton {}; struct Halwa {int time_taken; char *name; };";
    const char *query_string = "(binary_expression (number_literal)(number_literal)) @bin";
    
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
    if(VERBOSE){
        printf("Source Code: %s\n", source_code);
        printf("Query String: %s\n", query_string);
    }
    
    
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
            // printf("matched node: %s\n",node_string);
            free(node_string);
        }
    }
    const char *name = "Halwa";
    const char *from = "time_taken";
    const char *to   = "duration";
    // TSNode struct_node = find_struct_with_name(source_code,name,root_node);
    // if(ts_node_is_null(struct_node)){
    //     printf("THis shit is shit");
    //     printf("The struct is not there in the specified node");
    // } else {
    //     TSInputEdit edit = {0};
    //     change_field_in_struct(modified_source,struct_node,from, to, &edit);
    //     printf("%s\n", modified_source);
    // }
    print_tree(root_node);

    tree = change_struct_field(name, from, to, tree, root_node, source_code);

    root_node = ts_tree_root_node(tree);
    
    print_tree(root_node);
    free(node_string);
    ts_query_delete(query);
    ts_query_cursor_delete(query_cursor);
    ts_tree_delete(tree);
    ts_parser_delete(parser);

}