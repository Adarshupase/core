#include "cmd.h"
#include "utils.h"
#include "core.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>


/*
undo stack x, y
redo stack z 
 */


static Stack undo_stack = {0};
static Stack redo_stack = {0};

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
static int change_function_name_handler (int argc, char **argv, void *info) 
{
  (void) argc;
  const char *from = argv[0];
  const char *to = argv[1];
  TSTreeInfo *tree_info = (TSTreeInfo *) info;
  change_function_name(from, to, tree_info);
  return 0;
}

static int change_local_var_handler(int argc, char **argv, void *info)
{
  (void) argc;
  const char *function_name = argv[0];
  const char *from = argv[1];
  const char *to = argv[2];

  TSTreeInfo *tree_info = (TSTreeInfo *) info;
  change_variable_in_function(function_name, from, to, tree_info);
  return 0;
}




static int change_struct_name_handler(int argc, char **argv, void *info)
{
   (void)argc;
   const char *struct_name_from = argv[0];
   const char *struct_name_to = argv[1];

   TSTreeInfo *tree_info = (TSTreeInfo *) info;

   change_struct_name(struct_name_from, struct_name_to,tree_info);
   return 0;
}

static Command core_commands[] = {

  {CHANGE_STRUCT_FIELD, "change_struct_field",3,change_struct_field_handler},
  {CHANGE_STRUCT_NAME,"change_struct_name",2,change_struct_name_handler},
  {CHANGE_FUNCTION_NAME,"change_function_name",2, change_function_name_handler},
  {CHANGE_LOCAL_VAR, "change_local_var",3, change_local_var_handler}, 
};

static int get_command_id (const char *name)
{
  int total_commands = TOTAL_CORE_COMMANDS;

  if (strcmp(name, "undo")==0) {
	return UNDO_COMMAND;
  } else if (strcmp(name, "redo") == 0) {
	return REDO_COMMAND;
  }
  for (int i = 0; i < total_commands; i++) {
	if (strcmp(core_commands[i].command_name,name)==0) {
	  return core_commands[i].command_id;
	}
  }
  
  return -1;
}

void swap_args(int core_command_id, char **argv)
{
  switch(core_command_id) {
	
	case CHANGE_STRUCT_FIELD:
	case CHANGE_LOCAL_VAR: {
	  char *temp = argv[1];
	  argv[1] = argv[2];
	  argv[2] = temp;
	} break;
	  
	case CHANGE_STRUCT_NAME:
	case CHANGE_FUNCTION_NAME:{
	  char *temp = argv[0];
	  argv[0] = argv[1];
	  argv[1] = temp;
	} break;
  default:
	break;
	}
}

static char **copy_args(char *const *args, int arg_count)
{
  char **new_args = (char **) malloc (sizeof (char *) * arg_count);

  for (int i = 0;i < arg_count; i++) {
	int len = strlen(args[i]);
	new_args[i] = malloc (len + 1);
	memcpy(new_args[i],args[i],len);
	new_args[i][len] = '\0';
	
  }
  return new_args;
}
static void  undo (Parsed_Command *commands, TSTreeInfo *info)
{
  
  Parsed_Command *parsed_command = NULL;
  
  if (is_empty(&undo_stack)) {
	fprintf(stderr, "Nothing to undo\n");
	return;
  } 	
  int top_of_stack = top(&undo_stack);
  
  parsed_command = &commands[top_of_stack];
  
  int core_command_id = get_command_id(parsed_command->command);
  
  Command *core_command = &core_commands[core_command_id];
  char **temp_args = copy_args(parsed_command->args, parsed_command->number_of_arguments);
  swap_args(core_command_id, temp_args);
  
  int command_status = core_command->command_handler(parsed_command->number_of_arguments, temp_args, (void *)info);
  
  if (command_status != 0) {
    fprintf(stderr,
  		  "Error: command '%s' failed with code %d\n",
  		  parsed_command->command,
  		  command_status
  		  );
  } else {
    // push to redo stack because command succeded
	printf("UNDO: %s with args: ", parsed_command->command);
	for (int i = 0; i < parsed_command->number_of_arguments; i++)
	  printf("%d) %s",i,  temp_args[i]);
	pop(&undo_stack);
	push(&redo_stack,top_of_stack);
	
	printf(">> Pushed command: %s on to redo stack\n", parsed_command->command);
  }

  free(temp_args);
}

static void redo(Parsed_Command *commands, TSTreeInfo *info)
{
  
  Parsed_Command *parsed_command = NULL;
  
  if (is_empty(&redo_stack)) {
	fprintf(stderr, "Nothing to redo\n");
  } 	
  int top_of_stack = top(&redo_stack);
  
  parsed_command = &commands[top_of_stack];
  
  int core_command_id = get_command_id(parsed_command->command);
  
  Command *core_command = &core_commands[core_command_id];
  int command_status = core_command->command_handler(parsed_command->number_of_arguments, parsed_command->args, (void *)info);
  
  if (command_status != 0) {
    fprintf(stderr,
  		  "Error: command '%s' failed with code %d\n",
  		  parsed_command->command,
  		  command_status
  		  );
  } else {
	printf("REDO: %s with args: ", parsed_command->command);
	for (int i = 0; i < parsed_command->number_of_arguments; i++)
	  printf("%d) %s",i, parsed_command->args[i]);
	pop(&redo_stack);
	push(&undo_stack, top_of_stack);
	printf(">> Pushed command: %s on to undo stack\n", parsed_command->command);
  }
    
}


void print_commands(Parsed_Command *commands, int total) 
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

void execute_commands(Parsed_Command *commands, int total_commands, TSTreeInfo *info) 
{
    for(int i = 0; i < total_commands; i++) {
	  //search_for_command(commands[i].command, commands[i].number_of_arguments);
        int command_id  = get_command_id(commands[i].command);
		
	  
        if(command_id == -1) {
            fprintf(stderr, "Unknown command: %s\n",commands[i].command);
            exit(EXIT_FAILURE);
        }
		if (command_id == UNDO_COMMAND) {
		  undo(commands,info);
		  continue;
		} else if (command_id == REDO_COMMAND){
		  redo(commands,info);
		  continue;
		}
		
		Command core_command = core_commands[command_id];

        if(!core_command.command_handler) {
            fprintf(stderr,"Error: NO handler registered for command: %s",commands[i].command);
            exit(EXIT_FAILURE);
        }

        if(core_command.argument_count != commands[i].number_of_arguments){
            fprintf(stderr, "Argument Mismatch for '%s' (expected %d, got %d)\n",
                commands[i].command,core_command.argument_count,commands[i].number_of_arguments
            );
            exit(EXIT_FAILURE);
        }
        int command_status = core_command.command_handler(
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
        } else {
		  
		  printf("Executed the command: %s\n", commands[i].command);
		  push(&undo_stack,i);
		  printf("%s is on stack\n", commands[i].command);
		  if (!is_empty(&redo_stack)) {
			clear(&redo_stack);
			printf("Cleared Redo Stack\n");
		  }
		  
			
		  
		}
    }
}
/* PROBLEM: 
 * We got hit with a pretty nasty problem here we got to install undo() and redo() command but 
 * the problem is it's not like normal commands the arguments for this depends on the type of command we need to
 * undo. it's come to a point where we have to write undo routine for each of the command with the command_handler 
 * SOL1: 
 * we can push the Parsed_Command to stack directly which is easier but 
 * more memory consuming. and have to create a new stack struct for that.
 * SOL2: 
 * handle it like any other command argc = number_of_arguments, argv[0] = command_id, argv[1] = total_arguments_for_this_command, 
 * argv[2] .. The arguments. Exit if the command_id and associated atrributes don't match .

 */



// maybe i should have a global tree info whenever a command executes we can get tree info from that 
// sort of like a get tree info function like we are encapsulating tree info and adding getter and 
// setter oop all over again 
// @BEFORE(
//  1) parse commands into Parsed_Command[] 
//  2) pass the parse commands into execute_commands with TreeInfo as an argument 
//  3) then the execute_commands is responsible for convering strings to enums and then switching. this was simple. 
// )
// @AFTER (
// 1) Parse commands into Parsed_Command[] as usual.
// 2) But now instead of converting them into 3 arrays of enum's we have a hash table 
// {command_string : Command {number_of_arguments,function_pointer_to_handler}}
// 3) The same old logic as the so we make the handler require an additional argument which is the TSTreeInfo* which can be supplied by the execute_commands 
//)

Parsed_Command *parse_commands(const char *filename, int *total_commands)
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
    Parsed_Command *commands = calloc(capacity, sizeof(Parsed_Command));

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
void free_commands(Parsed_Command *commands, int total) 
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
