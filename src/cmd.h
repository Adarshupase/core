#ifndef __COMMAND_H 
#define __COMMAND_H

#include "core.h"


typedef struct {
  int command_id;
  char *command_name;
  int argument_count;
  int (*command_handler)(int argc, char **argv, void *info);
} Command;

typedef struct {
    
    char *command;
    char **args;
    int number_of_arguments;

} Parsed_Command;


typedef enum {
  
  CHANGE_STRUCT_FIELD = 0,
  CHANGE_STRUCT_NAME,
  CHANGE_FUNCTION_NAME,
  CHANGE_LOCAL_VAR,
  TOTAL_CORE_COMMANDS,
  UNDO_COMMAND,
  REDO_COMMAND
} Command_Id;



void print_commands(Parsed_Command *commands, int total) ;
// COMMAND_TYPE search_for_command(const char *command, int number_of_arguments);
Parsed_Command *parse_commands(const char *filename, int *total_commands);
void free_commands(Parsed_Command *commands,int total_commands);
void execute_commands(Parsed_Command *commands, int total_commands,TSTreeInfo *info);


#endif
