#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include "new.h"

typedef uint32_t u32;
#define CURSOR_MODE 0
#define VERBOSE 0
#define DEBUG_VERBOSE 1
#define MAX_DEFINION_SIZE 1000

const TSLanguage *tree_sitter_c(void);
char *array[MAX_DEFINION_SIZE] = {NULL};
int array_size = 0;

static void insert_into_array(const char *string) 
{   char *dup_string = strndup(string,strlen(string));
    if(array_size > MAX_DEFINION_SIZE-1) {
        return;
    }
    array[array_size++] = dup_string;
}

static bool find_in_array(const char *string) 
{
    for(int i = 0; i < array_size; i++) {
        if(strcmp(array[i],string) == 0) {
            return true;
        } 
    }
    return false;
}
static void cleanup()
{
    for(int i = 0; i < array_size; i++){
        if(array[i] != NULL) {
            free(array[i]);
            array[i] = NULL;
        }
    }
    array_size = 0;
}

static void find_all_struct_declarations(const char *struct_name, TSNode root_node,const char *modified_source) 
{
    String_Builder sb = {0};

    sb_append(&sb, "(declaration type: (struct_specifier name: (type_identifier) @struct_name) ");
    sb_append(&sb, "(#eq? @struct_name \"");
    sb_append(&sb, struct_name);
    sb_append(&sb, "\")) @struct_declaration");

    const char *query_string = sb.string;
    TSQueryError error_type; u32 error_offset;
    TSQuery *query = ts_query_new(
        tree_sitter_c(), 
        query_string, 
        strlen(query_string),
        &error_offset,
        &error_type
    );
    sb_free(&sb);


    TSQueryCursor *cursor = ts_query_cursor_new();
    TSQueryMatch match;

    ts_query_cursor_exec(cursor, query, root_node);

    while(ts_query_cursor_next_match(cursor,&match)) {
        TSNode named_node = {0}; 

        for(u32 i = 0; i < match.capture_count; i++) {
            u32 length;
            const char *cap = ts_query_capture_name_for_id(query,match.captures[i].index, &length);
            if(strcmp(cap,"struct_declaration") == 0) {
                TSNode node = match.captures[i].node;
                named_node = find_child_node_of_type(node,"identifier");
            }
        }
        if(!ts_node_is_null(named_node)){
            u32 start_byte = ts_node_start_byte(named_node);
            u32 end_byte   = ts_node_end_byte(named_node);
            u32 slice_size = end_byte - start_byte;
            
            char *string = strndup(modified_source + start_byte, slice_size);
            insert_into_array(string);
            free(string);
            string = NULL;
        }
    }

}

// definitions 


void sb_append(String_Builder *sb, const char *string) 
{
    int old_size = sb->size;
    int new_size = old_size + strlen(string);
    sb->string = realloc(sb->string, new_size + 1);
    strcpy(sb->string + old_size, string);
    sb->size = new_size;
}

void sb_free(String_Builder *sb) 
{
    free(sb->string);
    sb->string = NULL;
    sb->size = 0;
}

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

// recursivery print tree with identation 
void print_tree(TSNode root_node) 
{
    traverse(root_node, 0);
}

// print tree using cursor 
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


// traverse the tree recursively and debug the tree by 
// printing the type of nodes along with their source code 
void traverse_and_debug(TSNode node, int nest, const char *source ) 
{
    for(u32 i = 0; i < ts_node_child_count(node); i++) 
    {
        TSNode child = ts_node_child(node,i);
        u32 start = ts_node_start_byte(child);
        u32 end   = ts_node_end_byte(child);

        char *string = strndup(source + start,end - start);
        
        for(int i = 0; i < nest ; i++) {
            printf(" ");
        }
        if(DEBUG_VERBOSE){
            printf("%s : ",ts_node_type(child));
            printf("%s\n", string);
        } else {
            if(ts_node_is_named(child)){
                printf("%s : ",ts_node_type(child));
                 printf("%s\n", string);
            }
        }
        free(string);
        
        traverse_and_debug(child,nest + 1, source);
    }
}
// debug_tree prints tree nodes with their names so you can verify if the tree has indeed changed
// after editing the tree 
void debug_tree(TSNode root_node, const char *source) 
{
    traverse_and_debug(root_node,0, source);
}
void change_field_in_struct(char *modified,
                            TSNode node,
                            const char *from,
                            const char *to,
                            TSInputEdit *edit)
{
    
    const char *query_string = "(field_declaration) @field";

    // @TODO(make this as a macro or a function ) my code is already breaking i don't want to add more uncertainity
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

    ts_query_cursor_exec(cursor, query, node);
    while(ts_query_cursor_next_match(cursor, &match)){
        TSNode name_node = {0};
        for(u32 i = 0; i < match.capture_count; i++) {
            TSNode node = match.captures[i].node;
            name_node = find_child_node_of_type(node, "field_identifier");
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
                edit->new_end_byte = start_byte+ new_slice_size;
                edit->start_point = ts_node_start_point(name_node);
                edit->old_end_point = ts_node_end_point(name_node);
                TSPoint start = ts_node_start_point(name_node);

                edit->new_end_point = (TSPoint){
                    .row = start.row,
                    .column = start.column + new_slice_size
                };
                

                break;
            } 
        }
    }
    ts_query_cursor_delete(cursor); 
    ts_query_delete(query); 

}


void change_struct_field_in_program(char *modified_source, const char *struct_name, const char *from, 
                                    const char *to, TSInputEdit *edit, TSNode root_node)
{

    const char *query_string = "(field_expression argument: (identifier) @identifier_name field: (field_identifier) @field_name)";
    
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

    find_all_struct_declarations(struct_name, root_node, modified_source);
    ts_query_cursor_exec(cursor, query, root_node);
    while(ts_query_cursor_next_match(cursor, &match)){
        TSNode identifier_node = {0};
        TSNode field_node  = {0};
        for(u32 i = 0; i < match.capture_count; i++){
             u32 length;
             const char *cap = ts_query_capture_name_for_id( query, match.captures[i].index, &length );
             if ((strcmp(cap, "identifier_name") == 0))  identifier_node =  match.captures[i].node;
             else if (strcmp(cap, "field_name") == 0) field_node = match.captures[i].node; 
        }
        if(!ts_node_is_null(identifier_node)) {
            u32 start_byte = ts_node_start_byte(identifier_node);
            u32 end_byte = ts_node_end_byte(identifier_node);
            u32 slice_size = end_byte - start_byte;
            u32 field_start_byte = ts_node_start_byte(field_node);
            u32 field_end_byte = ts_node_end_byte(field_node);
            u32 field_size = field_end_byte - field_start_byte;
            char *string = strndup(modified_source + start_byte, slice_size);
            if(find_in_array(string)) {
                if (strncmp(modified_source + field_start_byte, from, field_size) == 0 && from[field_size] == '\0') 
                { 
                    size_t new_field_size = strlen(to);
                    if(new_field_size != field_size) {
                        memmove(
                            modified_source + field_start_byte + new_field_size,
                            modified_source + field_end_byte,   
                            strlen(modified_source) - field_end_byte + 1
                        );
                    }
                    memcpy(modified_source + field_start_byte, to, new_field_size); 
                    edit->start_byte = field_start_byte;
                    edit->old_end_byte = field_end_byte;
                    edit->new_end_byte = field_start_byte + new_field_size;
                    edit->start_point = ts_node_start_point(field_node);
                    edit->old_end_point = ts_node_end_point(field_node);
                    TSPoint start = ts_node_start_point(field_node);
                    edit->new_end_point = (TSPoint){
                        .row = start.row,
                        .column = start.column + new_field_size
                    };

                }
            }
            free(string);
            cleanup();

        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);

}
TSNode find_child_node_of_type(TSNode node, const char *type)
{
    for (u32 i = 0; i < ts_node_child_count(node); i++) {
        TSNode child = ts_node_child(node, i);

        if (strcmp(ts_node_type(child), type) == 0) {
            return child;
        }

        TSNode result = find_child_node_of_type(child, type);
        if (!ts_node_is_null(result)) {
            return result;
        }
    }

    return (TSNode){0};
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

void change_struct_field(const char *struct_name,
                    const char *from,
                    const char *to,
                    TSTreeInfo *info)
{
    TSNode root_node = ts_tree_root_node(info->tree);
    TSNode struct_node = find_struct_with_name(info->source_code,struct_name,root_node);
    if(ts_node_is_null(struct_node)) {
        return;
    }
    char *modified_source = malloc(strlen(info->source_code) + 1);

    strcpy(modified_source,info->source_code);
    TSInputEdit edit = {0};
    change_field_in_struct(modified_source, struct_node, from, to, &edit);
    ts_tree_edit(info->tree, &edit);
    change_struct_field_in_program(modified_source,struct_name, from, to, &edit, root_node);
    
    TSTree *new_tree = ts_parser_parse_string(
        info->parser, 
        info->tree,
        modified_source,
        strlen(modified_source)
    );
    ts_tree_delete(info->tree);
    info->source_code = modified_source;
    info->tree =  new_tree;
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

void ts_cleanup(TSTreeInfo *info) 
{
    ts_tree_delete(info->tree);
    ts_parser_delete(info->parser);
    info->tree = NULL;
    info->parser = NULL;
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
    

    printf("--------------------------------------------------------------------------------------------------------------\n");
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
    
    printf("----------------------------DEBUG_INFO--------------------------------------------------\n");
    printf("____________________________BEFORE-CHANGE________________________________________________\n");
    
    debug_tree(root_node,info.source_code);
    change_struct_field("Halwa","a","c",&info);
    change_struct_field("Halwa","c","d",&info);

    root_node = ts_tree_root_node(info.tree);
    printf("____________________________AFTER-CHANGE________________________________________________\n");
    debug_tree(root_node, info.source_code);
    
    
    free(node_string);
    ts_query_delete(query);
    ts_tree_cursor_delete(&tree_cursor);
    ts_query_cursor_delete(query_cursor);
    ts_cleanup(&info);

}

