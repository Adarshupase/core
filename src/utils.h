#ifndef __UTILS_H
#define __UTILS_H
#include <stdio.h>
typedef struct {
    char *string;
    int size;
} String_Builder;


void sb_append(String_Builder *sb, const char *string) ;
void sb_append_till(String_Builder *sb, const char *string, int length) ;
String_Builder sb_init() ;
void sb_free(String_Builder *sb);
void pretty_print_tree(const char *string);
void write_modified_string_to_file(const char *file_name, const char *source_string) ;
char *trim(char *s);
#endif // __UTILS_H