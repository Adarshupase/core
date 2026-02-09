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
#define TOTAL_CORE_COMMANDS 2
#define MAX_STRUCT_DECLARATIONS 1000
#define TS_NODE_SLICE(node, start, end, size) \
    do {                                     \
        (start) = ts_node_start_byte(node); \
        (end)   = ts_node_end_byte(node);   \
        (size)  = (end) - (start);          \
    } while (0)

char *array[MAX_DEFINION_SIZE] = {NULL};

int array_size = 0;
char *core_commands[TOTAL_CORE_COMMANDS] = {"change_struct_field", "change_struct_name"};
COMMAND_TYPE core_commands_enum[TOTAL_CORE_COMMANDS] = {CHANGE_STRUCT_FIELD, CHANGE_STRUCT_NAME};
int core_command_arguments[TOTAL_CORE_COMMANDS] = {3,2};

// function arguments in comments are referred to as @(arg_name)
// example:
/*
  get_node_type(Node node) 
  {
        // @(node) is a tree node.
        ....      
  }
*/


TSTree *get_new_tree(TSTreeInfo *info, char *modified_source){
    return ts_parser_parse_string(
        info->parser, 
        info->tree,
        modified_source,
        strlen(modified_source)
    );
}

void insert_into_array(const char *string) 
{   
    char *dup_string = strndup(string,strlen(string));
    if(array_size > MAX_DEFINION_SIZE-1) {
        return;
    }
    array[array_size++] = dup_string;
}

bool find_in_array(const char *string) 
{
    for(int i = 0; i < array_size; i++) {
        if(strcmp(array[i],string) == 0) {
            return true;
        } 
    }
    return false;
}
void cleanup()
{
    for(int i = 0; i < array_size; i++){
        if(array[i] != NULL) {
            free(array[i]);
            array[i] = NULL;
        }
    }
    array_size = 0;
}

TSQuery *struct_declaration_query(const char *struct_name)
{
    String_Builder sb = {0};
    sb_append(&sb, "(declaration type: (struct_specifier name: (type_identifier) @struct_name) ");
    sb_append(&sb, "(#eq? @struct_name \"");
    sb_append(&sb, struct_name);
    sb_append(&sb, "\")) @struct_declaration");

    const char *query_string = sb.string;
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
        fprintf(stderr,
            "Query error %d at offset %u\n",
             error_type, 
             error_offset
        );
        sb_free(&sb);
        return NULL;
    }
    // footgun or something


    return query;
    
}

static Hash_Table *find_all_struct_declarations(const char *struct_name, TSNode root_node,const char *modified_source) 
{
    TSQuery *query = struct_declaration_query(struct_name);
    
    TSQueryCursor *cursor = ts_query_cursor_new();
    TSQueryMatch match;
    Hash_Table *struct_declarations = create_table(MAX_DEFINION_SIZE);

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
            u32 start_byte, end_byte, slice_size;
            TS_NODE_SLICE(named_node,start_byte, end_byte,slice_size);
            
            char *string = strndup(modified_source + start_byte, slice_size);
            insert_into_table(struct_declarations,string,"1");
            // insert_into_array(string);
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
// change the field name in struct definition only 


// void edit_source_code(const char *to, char *modified, TSNode name_node,TSInputEdit *edit)
// {
//     u32 start_byte, end_byte, slice_size;
//     TS_NODE_SLICE(name_node,start_byte,end_byte,slice_size);
//     size_t new_slice_size = strlen(to);
//     if(new_slice_size != slice_size){
//         memmove(
//         modified + start_byte + new_slice_size,
//         modified + end_byte, 
//         strlen(modified) - end_byte + 1
//         );
//     }
//     memcpy(modified + start_byte, to, new_slice_size);
//     edit->start_byte = start_byte;
//     edit->old_end_byte = end_byte;
//     edit->new_end_byte = start_byte+ new_slice_size;
//     edit->start_point = ts_node_start_point(name_node);
//     edit->old_end_point = ts_node_end_point(name_node);
//     TSPoint start = ts_node_start_point(name_node);

//     edit->new_end_point = (TSPoint){
//         .row = start.row,
//         .column = start.column + new_slice_size
//     };
// }
// static void edit_source_code_raw(
//     const char *to, 
//     char **modified_ptr,
//     u32 start, 
//     u32 end)
// {
//     char *buffer = *modified_ptr;

//     size_t old_length = end - start;
//     size_t new_length = strlen(to);
//     size_t total_length = strlen(buffer);

//     if(new_length != old_length) {
//         size_t new_total = total_length- old_length + new_length + 1;

//         char *new_buffer = realloc(buffer,new_total);
//         assert(new_buffer);
//         buffer = new_buffer;

//         memmove(
//             buffer + start + new_length,
//             buffer + end,
//             total_length - end + 1
//         );

//         *modified_ptr = buffer;
//     }

//     memcpy(buffer + start , to, new_length);
// }

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
void print_commands(Core_Command *commands, int total) 
{
    for(int i = 0; i < total; i++) {
        printf("Command: %s\n", commands[i].command);
        printf("Args (%d): ", commands[i].number_of_arguments);
        for(int j = 0; j < commands[i].number_of_arguments; j++) {
            printf("%s", commands[i].args[j]);
            if(j + 1 < commands[i].number_of_arguments) {
                printf(", ");
            }
            printf("\n");
        }
    }
}


COMMAND_TYPE search_for_command(const char *string, int number_of_arguments){
    for(int i = 0; i < TOTAL_CORE_COMMANDS; i++) {
        if((strcmp(core_commands[i],string)==0)){
            if(core_command_arguments[i]==number_of_arguments){
                return core_commands_enum[i];
            } else {
                return ARGUMENT_MISMATCH;
            }
        }
    }
    return COMMAND_NOT_FOUND;
    
}
void execute_commands(Core_Command *commands, int total_commands, TSTreeInfo *info) 
{
    for(int i = 0; i < total_commands; i++) {
        COMMAND_TYPE command = search_for_command(commands[i].command, commands[i].number_of_arguments);
            switch (command)
            {
            case CHANGE_STRUCT_FIELD:
                change_struct_field(commands[i].args[0],commands[i].args[1],commands[i].args[2],info);
                
                break;
            case CHANGE_STRUCT_NAME:
                change_struct_name(commands[i].args[0],commands[i].args[1],info);
                break;
            case ARGUMENT_MISMATCH:
                fprintf(stderr,"command %s failed arguments mismatch\n",commands[i].command);
                return;
            case COMMAND_NOT_FOUND:
                fprintf(stderr,
                "Error: unknown command '%s'\n",
                commands[i].command);
            return;

            
            default:
                printf("%s\n",info->source_code);
                break;
            }
    }
    
}

Core_Command *parse_commands(const char *filename, int *total_commands)
{
    *total_commands = 0;
    char *source = read_entire_file(filename);
    if(source == NULL) {
        perror("Can't read file\n");
        return NULL;
    }
    int capacity = 0;
    for(char *p = source ; *p;) {
        while(*p && isspace((unsigned char)*p))p++;
        if(!*p) break;
        capacity++;
        while(*p && *p != '\n') p++;
    }
    if(capacity == 0) {
        free(source);
        return NULL;
    }
    Core_Command *commands = calloc(capacity, sizeof(Core_Command));

    char *copy = strdup(source);
    char *save_line;
    char *line = strtok_r(copy, "\n", &save_line);

    int count = 0;

    while(line) {
        char *l = trim(line);

        if(*l == '\0') {
            line = strtok_r(NULL, "\n", &save_line);
            continue;
        }
        char *open = strchr(l, '(');
        char *close = strchr(l, ')');

        if(!open || !close || close < open) {
            line = strtok_r(NULL, "\n",&save_line);
            continue;
        }
        *open = '\0';
        *close = '\0';

        commands[count].command = strdup(trim(l));
        char *args_str = trim(open + 1);
        int argc = 0;
        if(*args_str) {
            argc = 1;
            for(char *p = args_str; *p; p++) {
                if(*p == ',') argc++;
            }
        }
        commands[count].number_of_arguments = argc;
        commands[count].args = malloc(sizeof(char *) * argc);

        char *save_arg;
        char *arg = strtok_r(args_str,",",&save_arg);
        int i = 0;
        while(arg) {
            commands[count].args[i++] = strdup(trim(arg));
            arg = strtok_r(NULL, ",", &save_arg);
        }
        count++;

        line = strtok_r(NULL, "\n",&save_line);

    }
    free(copy);
    free(source);

    *total_commands = count;
    return commands; 
}

void free_commands(Core_Command *commands, int total) 
{
    for(int i = 0; i < total; i++) {
        free(commands[i].command);
        for(int j = 0; j < commands[i].number_of_arguments; j++) {
            free(commands[i].args[j]);
        }
        free(commands[i].args);
    }
    free(commands);
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


// void change_struct_name_in_program(const char *struct_name,const char *to, TSTreeInfo *info,char *modified_source)
// {
//     // TSTree *tree1 = get_new_tree(info,modified_source);// create a new tree 
//     // ts_tree_delete(info->tree); // delete the old tree 
//     // info->tree = tree1; // set the new tree in info 

//     // TSNode new_root = ts_tree_root_node(info->tree); // get the new root 

//     TSNode root_node = ts_tree_root_node(info->tree);
//     TSQuery *query = struct_declaration_query(struct_name);
//     TSQueryCursor *cursor = ts_query_cursor_new(); 
//     ts_query_cursor_exec(cursor, query, root_node); 
//     TSQueryMatch match;
//     while(ts_query_cursor_next_match(cursor,&match)){
//         for(u32 i = 0; i < match.capture_count; i++) {
//             u32 length;
//             const char *cap = ts_query_capture_name_for_id(query,match.captures[i].index,&length);
//             if(strcmp(cap,"struct_name")==0){
//                 TSNode name_node = match.captures[i].node;
//                 if(ts_node_is_null(name_node)) printf("Not FOUND\n");
//                 TSInputEdit edit = {0};
//                 edit_source_code(to,modified_source,name_node,&edit);
//                 ts_tree_edit(info->tree,&edit);
//                 TSTree *new_tree = get_new_tree(info,modified_source);
//                 ts_tree_delete(info->tree);
//                 info->tree = new_tree;
//                 info->source_code = modified_source;
//             }
//         }
//     }
//     ts_query_cursor_delete(cursor);
//     ts_query_delete(query);

// }

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

// void change_function_name(const char *function_name, const char *to, TSTreeInfo *info) 
// {
//     const char *query_string = "(function_declarator declarator:(identifier) @func_name)";
//     u32 error_offset;
//     TSQueryError error_type;
//     TSQuery *query = ts_query_new(
//         tree_sitter_c(), 
//         query_string, 
//         strlen(query_string),
//         &error_offset,
//         &error_type
//     );
// }
void change_struct_name(const char *struct_name,
                    const char *to,
                    TSTreeInfo *info)
{
    TSNode root_node = ts_tree_root_node(info->tree);
    TSNode struct_node = find_struct_with_name(info->source_code,struct_name, root_node);
    if(ts_node_is_null(struct_node)){
        return;
    }
    char *modified_source = malloc(strlen(info->source_code) + 1);
    strcpy(modified_source, info->source_code);
    change_name_in_struct_declaration(&modified_source, struct_node,to,info);
    change_struct_name_in_program(struct_name,to,info,modified_source);
}


// void change_struct_field(const char *struct_name,
//                     const char *from,
//                     const char *to,
//                     TSTreeInfo *info)
// {
//     TSNode root_node = ts_tree_root_node(info->tree);
//     TSNode struct_node = find_struct_with_name(info->source_code,struct_name,root_node);
//     if(ts_node_is_null(struct_node)) {
//         return;
//     }
//     char *modified_source = malloc(strlen(info->source_code) + 1);

//     strcpy(modified_source,info->source_code);
//     TSInputEdit edit = {0};
//     change_field_in_struct(modified_source, struct_node, from, to, &edit);
//     ts_tree_edit(info->tree, &edit);
//     //@TODO(field doesn't change when struct is declared using {struct Type identifier = {.x = 3, .y = 4})}
//     TSTree *tree1 = get_new_tree(info,modified_source);
//     ts_tree_delete(info->tree);
//     info->tree = tree1;

//     TSNode new_root = ts_tree_root_node(info->tree);
//     change_struct_field_in_program(modified_source,struct_name, from, to, &edit, new_root);
//     // there should be ts_tree_edit(here)
//     ts_tree_edit(info->tree,&edit);

//     TSTree *tree2 = get_new_tree(info,modified_source);
//     ts_tree_delete(info->tree);
//     info->source_code = modified_source;
//     info->tree =  tree2;
// }

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
