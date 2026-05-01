#include <stdio.h>
#include <string.h> 
#include "../src/core.h"
#include "../src/utils.h"


void print_captures(const char* query_string,const char *source,  TSNode root_node)
{
  Query_Context context = create_query_context(query_string, root_node);
    TSQueryMatch match;

    TSNode node = {0};
    while (ts_query_cursor_next_match(context.cursor, &match)) {
      for (u32 i = 0; i < match.capture_count; i++) {
	printf("----------CAPTURE %d ----------\n", i);
	u32 length;
	const char *cap = ts_query_capture_name_for_id(context.query, match.captures[i].index, &length);
	node = match.captures[i].node;
	u32 start, end, slice_size;

	if (!ts_node_is_null(node)) {
	  TS_NODE_SLICE(node, start, end, slice_size);
	  printf("%s: ", cap);
	  printf("%.*s\n",slice_size, source + start);
	}
      }
    }
}
int main ()
{
  const char *query_string = "[(declaration type: (struct_specifier name: (type_identifier) @struct.name ) declarator:(identifier) @type.name)@complete"
    "(type_definition type: (struct_specifier name: (type_identifier) @struct.name) declarator: (type_identifier) @type.name) @typedef]";
					      
    TSParser *parser = ts_parser_new();
    ts_parser_set_language(parser, tree_sitter_c());
    const char *source_code = read_entire_file("test.c");
    char *modified_source = malloc(strlen(source_code) + 1);
    strcpy(modified_source,source_code);

    TSTree *tree = ts_parser_parse_string(
        parser,
        NULL,
        source_code,
        strlen(source_code)
    );

    TSTreeInfo info = {
        .tree = tree,
        .source_code = source_code,
        .parser = parser
    };

    TSNode root_node = ts_tree_root_node(info.tree);

    print_captures(query_string, source_code, root_node);

    return 0;

    
    
}
