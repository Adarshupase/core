#ifndef __UTILS_H
#define __UTILS_H
#define INITIAL_TABLE_SIZE 1000
#define INITIAL_STACK_SIZE 20
#include <stdio.h>
#include <stdbool.h>

// this should have a capacity but we will look in the future.  
typedef struct {
    char *string;
    size_t size;
} String_Builder;

typedef struct stack_s
{
  int *items;
  size_t size;
  size_t capacity;
} Stack;



/*
let me give my two cents on how hash tables work

first you have a Hash_Item and then you search for a place within the table 
by hashing the value 
like for example if you have 
"core" you have to hash it and generate a index within the array 
so hash function so how should we incorporate a hash function for us
size_t hash(string source){
    size_t val = 0;
    for(c : source) {
        val = val + c;
    }
}
what {x,y} -> index [43] there is nothing at index 43 we just insert it
now if something is present at index 43 while(not_empty(hash_table[index]))index++;    
*/



/* String_Builder specific functions */
void sb_append(String_Builder *sb, const char *string) ;
void sb_append_till(String_Builder *sb, const char *string, size_t length) ;
String_Builder sb_init() ;
void sb_free(String_Builder *sb);

/* Stack specific functions */
void push(Stack *stack, int val);
void pop(Stack *stack);
int top(Stack *stack);
bool is_empty(Stack *stack);
void clear(Stack *stack);
 
char *read_entire_file(const char *file_path);
void pretty_print_tree(const char *string);
void write_modified_string_to_file(const char *file_name, const char *source_string) ;
char *trim(char *s);
#endif // __UTILS_H
