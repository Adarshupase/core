#include "utils.h"
#include <stdio.h>

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