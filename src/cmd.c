#include "cmd.h"
#include "hash_table.h"
#include "utils.h"
#include "core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define TOTAL_CORE_COMMANDS 2

static Hash_Table *command_table = NULL;

static int change_struct_name_handler(int argc, char **argv, void *info)
{
   (void)argc;
   const char *struct_name_from = argv[0];
   const char *struct_name_to = argv[1];

   TSTreeInfo *tree_info = (TSTreeInfo *) info;

   change_struct_name(struct_name_from, struct_name_to,tree_info);
   return 0;
}


static int change_struct_field_handler(int argc, char **argv, void *info) 
{
    (void)argc;
    const char *struct_name = argv[0];
    const char *field_from = argv[1];
    const char *field_to  = argv[2];
    TSTreeInfo *tree_info = (TSTreeInfo *) info;
    change_struct_field(struct_name, field_from, field_to, tree_info);
    return 0;
}


void initialize_commands()
{
    if(command_table) return;
    command_table = create_table(20);
    Command *change_struct_field_command = malloc(sizeof(*change_struct_field_command));
    change_struct_field_command->argument_count = 3;
    change_struct_field_command->command_handler = change_struct_field_handler;
    insert_into_table(command_table,"change_struct_field",change_struct_field_command);
    Command *change_struct_name_command = malloc(sizeof(*change_struct_name_command));
    change_struct_name_command->argument_count = 2;
    change_struct_name_command->command_handler = change_struct_name_handler;
    insert_into_table(command_table,"change_struct_name",change_struct_name_command);

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

void execute_commands(Core_Command *commands, int total_commands, TSTreeInfo *info) 
{
    for(int i = 0; i < total_commands; i++) {
        // COMMAND_TYPE command = search_for_command(commands[i].command, commands[i].number_of_arguments);
        Command *command = get_from_table_or_null(command_table,commands[i].command);
        if(!command) {
            fprintf(stderr, "Unknown command: %s\n",commands[i].command);
            exit(EXIT_FAILURE);
        }

        if(!command->command_handler) {
            fprintf(stderr,"Error: NO handler registered for command: %s",commands[i].command);
            exit(EXIT_FAILURE);
        }

        if(command->argument_count != commands[i].number_of_arguments){
            fprintf(stderr, "Argument Mismatch for '%s' (expected %d, got %d)\n",
                commands[i].command,command->argument_count,commands[i].number_of_arguments
            );
            exit(EXIT_FAILURE);
        }
        int command_status = command->command_handler(
            commands[i].number_of_arguments,
            commands[i].args,
            (void *) info
        );

        if(command_status != 0) {
            fprintf(
                stderr,
                "Error: command '%s' failed with code %d\n",
                commands[i].command,
                command_status
            );
            exit(EXIT_FAILURE);
        }
    }

    destroy_table(command_table);
     
}




// maybe i should have a global tree info whenever a command executes we can get tree info from that 
// sort of like a get tree info function like we are encapsulating tree info and adding getter and 
// setter oop all over again 
// @BEFORE(
//  1) parse commands into Core_Command[] 
//  2) pass the parse commands into execute_commands with TreeInfo as an argument 
//  3) then the execute_commands is responsible for convering strings to enums and then switching. this was simple. 
// )
// @AFTER (
// 1) Parse commands into Core_Command[] as usual.
// 2) But now instead of converting them into 3 arrays of enum's we have a hash table 
// {command_string : Command {number_of_arguments,function_pointer_to_handler}}
// 3) The same old logic as the so we make the handler require an additional argument which is the TSTreeInfo* which can be supplied by the execute_commands 
//)

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