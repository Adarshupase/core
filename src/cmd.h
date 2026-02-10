#ifndef __COMMAND_H 
#define __COMMAND_H

#include "core.h"


typedef struct {
    int argument_count;
    int (*command_handler)(int argc, char **argv, void *info);
} Command;

typedef struct {
    char *command;
    char **args;
    int number_of_arguments;

} Core_Command;

void print_commands(Core_Command *commands, int total) ;
// COMMAND_TYPE search_for_command(const char *command, int number_of_arguments);
Core_Command *parse_commands(const char *filename, int *total_commands);
void free_commands(Core_Command *commands,int total_commands);
void execute_commands(Core_Command *commands, int total_commands,TSTreeInfo *info);
void initialize_commands();

#endif
