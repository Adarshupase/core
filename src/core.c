#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>

#include "hash_table.h"
#include "core.h"

// static int times = 0;

typedef uint32_t u32;
#define DEBUG_VERBOSE 1
#define MAX_DEFINION_SIZE 1000
#define MAX_STRUCT_DECLARATIONS 1000
#define TS_NODE_SLICE(node, start, end, size) \
    do {                                     \
        (start) = ts_node_start_byte(node); \
        (end)   = ts_node_end_byte(node);   \
        (size)  = (end) - (start);          \
    } while (0)


// function arguments in comments are referred to as @(arg_name)
// example:
/*
  get_node_type(Node node) 
  {
        // @(node) is a tree node.
        ....      
  }
*/
typedef enum {
    FUNCTION_DECLARATION,
    FUNCTION_DEFINITION,
    STRUCT_DEFINITION,
} Node_Type;
const char *get_error_type(TSQueryError error_type) 
{
    switch(error_type) {
        case TSQueryErrorNone:      return "TSQueryErrorNone";
        case TSQueryErrorSyntax:    return "TSQueryErrorSyntax";
        case TSQueryErrorNodeType:  return "TSQueryErrorNodeType";
        case TSQueryErrorField:     return "TSQueryErrorField";
        case TSQueryErrorCapture:   return "TSQueryErrorCapture";
        case TSQueryErrorStructure: return "TSQueryErrorStructure";
        case TSQueryErrorLanguage:  return "TSQueryErrorLanguage";
        default: return "UNKNOWN";
    }
}

void query_error(const char *string, TSQueryError error_type, u32 error_offset) 
{
    fprintf(stderr,
    "Error: Creating query from string %s of type %s at offset %d\n",
        string,
        get_error_type(error_type),
        error_offset
    );
    exit(EXIT_FAILURE);
}

TSTree *get_new_tree(TSTreeInfo *info, char *modified_source){
    return ts_parser_parse_string(
        info->parser, 
        info->tree,
        modified_source,
        strlen(modified_source)
    );
}






static Hash_Table *find_all_struct_declarations(const char *struct_name, TSNode root_node,const char *modified_source) 
{
    const char *query_string = "(declaration type: (struct_specifier name: (type_identifier) @struct_name) declarator: (identifier) @var_name) ";
    
    u32 error_offset;
    TSQueryError error_type;
    TSQuery *query = ts_query_new(
        tree_sitter_c(), 
        query_string, 
        strlen(query_string),
        &error_offset,
        &error_type
    );

    if(!query) {
        query_error(query_string,error_type,error_offset);
    }
    TSQueryCursor *cursor = ts_query_cursor_new();
    TSQueryMatch match;
    Hash_Table *struct_declarations = create_table(MAX_DEFINION_SIZE);

    ts_query_cursor_exec(cursor, query, root_node);

    while(ts_query_cursor_next_match(cursor,&match)) {
        TSNode struct_named_node = {0};
        TSNode var_named_node = {0}; 
        for(u32 i = 0; i < match.capture_count; i++) {
            u32 length;
            const char *cap = ts_query_capture_name_for_id(query,match.captures[i].index, &length);
            if(strcmp(cap,"struct_name") == 0) {
                struct_named_node = match.captures[i].node;
            } else if(strcmp(cap,"var_name")==0) {
                var_named_node = match.captures[i].node;
            }
        }
        if(!ts_node_is_null(struct_named_node)){
            u32 start_byte, end_byte, slice_size;
            TS_NODE_SLICE(struct_named_node,start_byte, end_byte,slice_size);
            char *string = strndup(modified_source + start_byte, slice_size);
            if(strcmp(string,struct_name) ==0) {
                u32 start_byte,end_byte,slice_size;
                TS_NODE_SLICE(var_named_node,start_byte,end_byte,slice_size);
                char *string = strndup(modified_source + start_byte,slice_size);
                printf("----------------%s------------------",string);
                insert_into_table(struct_declarations,string,(void *)1);
            }
            free(string);
            string = NULL;
        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    return struct_declarations;

}

// definitions 




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
    u32 start, end , slice_size;
    TS_NODE_SLICE(node,start, end, slice_size);

    char *string = strndup(source + start, slice_size);
    if(ts_node_is_named(node)){
        printf("%s : ",ts_node_type(node));
        printf("%s\n", string);
    }
    free(string);

    for(u32 i = 0; i < ts_node_child_count(node); i++) 
    {
        TSNode child = ts_node_child(node,i);
        u32 start, end, slice_size;
        TS_NODE_SLICE(child, start, end, slice_size);
        char *string = strndup(source + start,slice_size);
        
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


static TSPoint point_from_byte_offset(const char *source, u32 byte_offset)
{
    TSPoint point = {0, 0};
    for (u32 i = 0; i < byte_offset && source[i]; i++) {
        if (source[i] == '\n') {
            point.row++;
            point.column = 0;
        } else {
            point.column++;
        }
    }
    return point;
}

static TSPoint point_advance_by_string(TSPoint start, const char *s)
{
    TSPoint p = start;
    for (; *s; s++) {
        if (*s == '\n') {
            p.row++;
            p.column = 0;
        } else {
            p.column++;
        }
    }
    return p;
}

static void edit_source_code_raw(
    const char *to,
    char **modified_ptr,
    u32 start,
    u32 end
) {
    char *buffer = *modified_ptr;

    size_t old_len   = end - start;
    size_t new_len   = strlen(to);
    size_t total_len = strlen(buffer);

    if (new_len != old_len) {
        size_t new_total = total_len - old_len + new_len + 1;

        char *new_buffer = realloc(buffer, new_total);
        assert(new_buffer);

        buffer = new_buffer;
        *modified_ptr = buffer;

        memmove(
            buffer + start + new_len,
            buffer + end,
            total_len - end + 1   // includes '\0'
        );
    }

    memcpy(buffer + start, to, new_len);

    buffer[total_len - old_len + new_len] = '\0';
}

static void edit_source_code(
    const char *to,
    char **modified_ptr,
    TSNode name_node,
    TSInputEdit *edit
) {
    char *modified = *modified_ptr;

    u32 start, end, old_len;
    TS_NODE_SLICE(name_node, start, end, old_len);

    size_t new_len = strlen(to);
    size_t total_len = strlen(modified);

    if (new_len != old_len) {
        size_t new_total = total_len - old_len + new_len + 1;

        modified = realloc(modified, new_total);
        assert(modified);

        memmove(
            modified + start + new_len,
            modified + end,
            total_len - end + 1
        );

        *modified_ptr = modified;
    }

    memcpy(modified + start, to, new_len);

    edit->start_byte = start;
    edit->old_end_byte = end;
    edit->new_end_byte = start + new_len;
    edit->start_point = ts_node_start_point(name_node);
    edit->old_end_point = ts_node_end_point(name_node);

    TSPoint sp = ts_node_start_point(name_node);
    edit->new_end_point = (TSPoint){
        .row = sp.row,
        .column = sp.column + new_len
    };
}

void change_field_in_struct(char **modified_ptr,
                            TSNode node,
                            const char *from,
                            const char *to,
                            TSInputEdit *edit)
{
    char *modified = *modified_ptr;
    const char *query_string = "(field_declaration) @field_declaration";

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
        TSNode field_declaration_node   = match.captures[0].node;

        u32 n = ts_node_named_child_count(field_declaration_node);
        for(u32 i = 0; i < n; i++){
            TSNode child_node = ts_node_named_child(field_declaration_node,i);
            TSNode name_node = find_child_node_of_type(child_node,"field_identifier");
            if(ts_node_is_null(name_node)) {continue;}
            u32 start_byte, end_byte, slice_size;
            TS_NODE_SLICE(name_node, start_byte, end_byte,slice_size);
            if (strncmp(modified + start_byte, from, slice_size) == 0 && from[slice_size] == '\0')
            {
                edit_source_code(to, modified_ptr, name_node, edit);
                ts_query_cursor_delete(cursor);
                ts_query_delete(query);
                return;
            }
        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
}



// change the field name in the whole program wherever the struct type is declared and used
// @TODO(this is currently inefficient as we need to check for the presence of the struct identifier 
// with the struct name type using a linear search )
void change_struct_field_in_program(char **modified_source_ptr, const char *struct_name, const char *from,
                                    const char *to, TSInputEdit *edit, TSNode root_node)
{
    char *modified_source = *modified_source_ptr;
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

    Hash_Table *struct_declarations =  find_all_struct_declarations(struct_name, root_node, modified_source);

    // this finds all the struct declarations of type struct_name variable_name and stores the variable name as a string and then 
    // goes through the program and finds the wherever the variable_name is used to 
    // access a field if the field_is @(from) and change it to @(to) 
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
            u32 start_byte, end_byte, slice_size,field_start_byte, field_end_byte,field_size;
            TS_NODE_SLICE(identifier_node, start_byte, end_byte,slice_size);
            TS_NODE_SLICE(field_node, field_start_byte, field_end_byte, field_size);
            char *string = strndup(modified_source + start_byte, slice_size);
            if(get_from_table_or_null(struct_declarations,string)) {
                if (strncmp(modified_source + field_start_byte, from, field_size) == 0 && from[field_size] == '\0')
                {
                    edit_source_code(to, modified_source_ptr, field_node, edit);
                    free(string);
                    // cleanup();
                    destroy_table(struct_declarations);
                    ts_query_cursor_delete(cursor);
                    ts_query_delete(query);
                    return;
                }
            }
            free(string);
        }
    }
    destroy_table(struct_declarations);
    // cleanup();
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
}


TSNode find_child_node_of_type(TSNode node, const char *type)
{
    if(strcmp(ts_node_type(node),type)==0) return node;
    
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
            u32 start, end , slice_size;
            TS_NODE_SLICE(name_node, start, end, slice_size);
            if (strncmp(source + start, struct_name, slice_size) == 0 && struct_name[end - start] == '\0') 
            { 
                result = struct_node; 
                break; 
            } 
        } 
    } 
    if(!ts_node_is_null(result)) {
        printf("Node is not null\n");
    }
    ts_query_cursor_delete(cursor); 
    ts_query_delete(query); 
    return result; 
}
// Source - https://stackoverflow.com/a
// Posted by user529758, modified by community. See post 'Timeline' for change history
// Retrieved 2026-01-27, License - CC BY-SA 4.0

void change_name_in_struct_declaration(
    char **modified_ptr,
    TSNode struct_node,
    const char *to,
    TSTreeInfo *info)
{
    TSInputEdit edit= {0};
    TSNode name_node = find_child_node_of_type(struct_node,"type_identifier");
    edit_source_code(to, modified_ptr, name_node,&edit);
    ts_tree_edit(info->tree,&edit);
    TSTree *new_tree = get_new_tree(info, *modified_ptr);
    ts_tree_delete(info->tree);
    info->tree =  new_tree;
    info->source_code = *modified_ptr;

}



void change_struct_name_in_program(const char *struct_name,const char *to, TSTreeInfo *info,char *modified_source)
{
    // TSTree *tree1 = get_new_tree(info,modified_source);// create a new tree 
    // ts_tree_delete(info->tree); // delete the old tree 
    // info->tree = tree1; // set the new tree in info 

    // TSNode new_root = ts_tree_root_node(info->tree); // get the new root 
    const char *query_string =     "(struct_specifier name: (type_identifier) @struct_name) declarator: (identifier)";
    TSNode root_node = ts_tree_root_node(info->tree);
    u32 error_offset;
    TSQueryError error;
    TSQuery *query = ts_query_new(
        tree_sitter_c(),
        query_string,
        strlen(query_string),
        &error_offset,
        &error
    );
    if(!query) {
        fprintf(stderr, 
        "Error creating query ");
    }

    TSQueryCursor *cursor = ts_query_cursor_new(); 
    ts_query_cursor_exec(cursor, query, root_node); 
    EditSpan nodes[MAX_STRUCT_DECLARATIONS];
    int count = 0;
    TSQueryMatch match;
    while(ts_query_cursor_next_match(cursor,&match)){
        for(u32 i = 0; i < match.capture_count; i++) {
            u32 length;
            const char *cap = ts_query_capture_name_for_id(query,match.captures[i].index,&length);
            if(strcmp(cap,"struct_name")==0){
                TSNode named_node = match.captures[i].node;
                u32 start_byte, end_byte, slice_size;
                TS_NODE_SLICE(named_node,start_byte, end_byte,slice_size);
                char *string = strndup(modified_source + start_byte, slice_size);
                if(strcmp(struct_name,string)==0){
                    nodes[count++] = (EditSpan){start_byte,end_byte};
                }
                free(string);
                string = NULL;
            }
        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);

    if(count == 0) return;

    size_t to_len = strlen(to);
    for(int i = count-1; i >= 0; i--){
        TSInputEdit edit = {0};
        edit.start_byte = nodes[i].start;
        edit.old_end_byte = nodes[i].end;
        edit.new_end_byte = nodes[i].start + (u32)to_len;
        edit.start_point = point_from_byte_offset(modified_source, nodes[i].start);
        edit.old_end_point = point_from_byte_offset(modified_source, nodes[i].end);
        edit.new_end_point = point_advance_by_string(edit.start_point, to);

        edit_source_code_raw(to, &modified_source, nodes[i].start, nodes[i].end);
        ts_tree_edit(info->tree, &edit);
    }
    TSTree *new_tree = get_new_tree(info,modified_source);
    ts_tree_delete(info->tree);
    info->tree = new_tree;
    info->source_code = modified_source;
    // printf("RUNNING %dth tiMe\n",times++);
    // debug_tree(ts_tree_root_node(info->tree),info->source_code);
}


void change_struct_name(const char *struct_name,
                    const char *to,
                    TSTreeInfo *info)
{
    TSNode root_node = ts_tree_root_node(info->tree);
    TSNode struct_node = find_struct_with_name(info->source_code,struct_name, root_node);
    if(ts_node_is_null(struct_node)){
        fprintf(stderr,
        "No struct with name: %s exiting....\n",struct_name);
        exit(EXIT_FAILURE);
    }
    char *modified_source = malloc(strlen(info->source_code) + 1);
    strcpy(modified_source, info->source_code);
    change_name_in_struct_declaration(&modified_source, struct_node,to,info);
    change_struct_name_in_program(struct_name,to,info,modified_source);
}



static TSNode find_node_with_name(
    Node_Type node_type,
    const char *source,
    const char *node_name,
    TSNode root_node)
{
    const char *query_string;
    const char *child_node;
    const char *named_string;

    switch (node_type)
    {
    case FUNCTION_DECLARATION:
        {
            query_string = "(declaration)  @decl";
            child_node   = "function_declarator";
            named_string = "identifier";
        }
        break;
    case FUNCTION_DEFINITION:
        {
            query_string = "(function_definition) @decl";
            child_node   = "function_declarator";
            named_string = "identifier";
        }
        break;
    case STRUCT_DEFINITION:
        {
            query_string = "(struct_specifier) @decl";
            child_node   = "struct_specifier";
            named_string = "type_identifier";
        }
        break;
    default:
        break;
    }
     
    u32 error_offset;
    TSQueryError error_type;

    TSQuery *query = ts_query_new(
        tree_sitter_c(),
        query_string,
        strlen(query_string),
        &error_offset,
        &error_type
    );

    if(!query) {
        query_error(query_string,error_type,error_offset);
    }
    TSQueryCursor *cursor = ts_query_cursor_new();
    ts_query_cursor_exec(cursor,query,root_node);

    TSQueryMatch match;
    TSNode result = {0};

    while(ts_query_cursor_next_match(cursor,&match)) {
        TSNode name_node = {0};
        TSNode parent_node = {0};

        for(u32 i = 0; i < match.capture_count; ++i) {
            u32 length;
            const char *cap = ts_query_capture_name_for_id(
                query,
                match.captures[i].index,
                &length 
            );
            if(strcmp(cap,"decl") == 0) {
                TSNode decl_node = match.captures[i].node;
                TSNode maybe_named_node = find_child_node_of_type(decl_node,child_node);

                if(!ts_node_is_null(maybe_named_node)){
                    parent_node = maybe_named_node;
                }
            }
        }
        if(!ts_node_is_null(parent_node)){
            u32 start,end,slice_size;
            name_node = find_child_node_of_type(parent_node,named_string);
            if(ts_node_is_null(name_node)) {
                fprintf(stderr,
                "No named child for the node");
            }
            TS_NODE_SLICE(name_node,start,end,slice_size);
            if (strncmp(source + start, node_name, slice_size) == 0 && node_name[slice_size] == '\0') 
            {
                result = parent_node;
                break;
            }
        }
    }
    ts_query_cursor_delete(cursor);
    ts_query_delete(query);
    return result;
}
static void change_function_name_in_declaration_or_definition(char **modified_ptr,
    TSNode function_node,
    const char *to,
    TSTreeInfo *info) 
{
    TSInputEdit edit = {0};
    TSNode named_node = find_child_node_of_type(function_node,"identifier");
    edit_source_code(to,modified_ptr,named_node,&edit);
    ts_tree_edit(info->tree,&edit);
    TSTree *new_tree = get_new_tree(info,*modified_ptr);
    info->tree = new_tree;
    info->source_code = *modified_ptr;
}
void change_function_name(const char *function_name, const char *to, TSTreeInfo *info) 
{
    printf("Change function name\n");
    TSNode root_node = ts_tree_root_node(info->tree);
    printf("%s\n",info->source_code);
    TSNode declaration_node = find_node_with_name(FUNCTION_DECLARATION, info->source_code,function_name,root_node);
    if(ts_node_is_null(declaration_node)){
        fprintf(stderr,
        "No function with name %s exiting...\n",function_name);
        exit(EXIT_FAILURE);
    }
    char *modified_source = malloc(strlen(info->source_code) + 1);
    strcpy(modified_source,info->source_code);
    change_function_name_in_declaration_or_definition(&modified_source,declaration_node,to,info);
    TSNode definition_node = find_node_with_name(FUNCTION_DEFINITION, info->source_code,function_name,root_node);
    if(ts_node_is_null(definition_node)){
        fprintf(stderr,
        "No function with name %s exiting...\n",function_name);
        exit(EXIT_FAILURE);
    }
    char *modified_source2 = malloc(strlen(info->source_code) + 1);
    strcpy(modified_source2,info->source_code);
    change_function_name_in_declaration_or_definition(&modified_source2,definition_node,to,info);

    // change_function_name_in_program

}

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
    change_field_in_struct(&modified_source, struct_node, from, to, &edit);
    ts_tree_edit(info->tree, &edit);
    //@TODO(field doesn't change when struct is declared using {struct Type identifier = {.x = 3, .y = 4})}
    TSTree *tree1 = get_new_tree(info, modified_source);
    ts_tree_delete(info->tree);
    info->tree = tree1;

    TSNode new_root = ts_tree_root_node(info->tree);
    change_struct_field_in_program(&modified_source, struct_name, from, to, &edit, new_root);
    // there should be ts_tree_edit(here)
    ts_tree_edit(info->tree,&edit);

    TSTree *tree2 = get_new_tree(info,modified_source);
    ts_tree_delete(info->tree);
    info->source_code = modified_source;
    info->tree =  tree2;
}

void ts_cleanup(TSTreeInfo *info) 
{
    ts_tree_delete(info->tree);
    ts_parser_delete(info->parser);
    info->tree = NULL;
    info->parser = NULL;

}
