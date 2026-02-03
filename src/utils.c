
#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define HASH_TABLE_SIZE 50


String_Builder sb_init() 
{
    String_Builder sb;
    sb.string = NULL;
    sb.size = 0;
    return sb;
}

void sb_append(String_Builder *sb, const char *string) 
{
    if(!string) return;
    size_t length = strlen(string);
    size_t old_size = sb->size;
    size_t new_size = old_size + length;
    char *new_buf = realloc(
        sb->string,
        new_size + 1
    );
    if(!new_buf) {
        perror("realloc");
        exit(1);
    }
    sb->string = new_buf;
    memcpy(sb->string + old_size, string, length);
    sb->string[new_size] = '\0';
    sb->size = new_size;
}

void sb_append_till(String_Builder *sb, const char *string, size_t length) 
{
    if(!string || length <= 0) return;

    size_t old_size = sb->size;
    size_t new_size = old_size + length;

    char *new_buf = realloc(
        sb->string, 
        new_size + 1
    );
    if(!new_buf) {
        perror("realloc");
        exit(1);
    }
    sb->string = new_buf;

    memcpy(sb->string + old_size, string, length);
    sb->string[new_size] = '\0';
    sb->size = new_size;
}

void sb_free(String_Builder *sb) 
{
    free(sb->string);
    sb->string = NULL;
    sb->size = 0;
}

void pretty_print_tree(const char *string){
    int indent = 0;
    for (const char *p = string; *p != '\0'; p++) {
        if (*p == '(') {
            printf("\n");
            for (int i = 0; i < indent; i++) {
                printf("  ");
            }
            indent++;
            printf("(");
        } else if (*p == ')') {
            indent--;
            printf(")");
        } else if (*p == ' ') {
            printf(" ");
        } else {
            printf("%c", *p);
        }
    }
    printf("\n");
}

void write_modified_string_to_file(const char *file_name, const char *source_string)
{
    String_Builder sb = sb_init();

    size_t len = strlen(file_name);

    if(len > 2 && strcmp(file_name + len -2, ".c")==0){
        sb_append_till(&sb, file_name, len-2);
    } else {
        sb_append(&sb,file_name);
    }
    sb_append(&sb,"_modified.c");

    FILE *file = fopen(sb.string, "w");
    if (file == NULL) {
        perror("Failed to open file for writing");
        sb_free(&sb);
        return;
    }
    fprintf(file, "%s", source_string);
    fclose(file);
    sb_free(&sb);
}
char *read_entire_file(const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if(!f) return NULL;
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  /* same as rewind(f); */

    char *string = malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    return string;
}
char *trim(char *s) 
{
    while(isspace((unsigned char)*s)) s++;
    if(*s == 0) return s;

    char *end = s + strlen(s) - 1;
    while(end > s && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return s;
}


