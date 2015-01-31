#include "strm.h"
#include "node.h"
#include <stdio.h>

static void
dump_node(strm_node* node, int indent) {
  int i;
  for (i = 0; i < indent; i++)
    putchar(' ');

  if (!node) {
    printf("NIL\n");
    return;		  
  }

  switch (node->type) {
  case STRM_NODE_ARGS:
    {
      strm_array* arr0 = node->value.v.p;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
    break;
  case STRM_NODE_IF:
    {
      printf("IF:\n");
      dump_node(((strm_node_if*)node->value.v.p)->cond, indent+1);
      for (i = 0; i < indent; i++)
        putchar(' ');
      printf("THEN:\n");
      dump_node(((strm_node_if*)node->value.v.p)->compstmt, indent+1);
      strm_node* opt_else = ((strm_node_if*)node->value.v.p)->opt_else;
      if (opt_else != NULL) {
        for (i = 0; i < indent; i++)
          putchar(' ');
        printf("ELSE:\n");
        dump_node(opt_else, indent+1);
      }
    }
    break;
  case STRM_NODE_EMIT:
    printf("EMIT:\n");
    dump_node((strm_node*) node->value.v.p, indent+1);
    break;
  case STRM_NODE_OP:
    printf("OP:\n");
    dump_node(((strm_node_op*) node->value.v.p)->lhs, indent+1);
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    puts(((strm_node_op*) node->value.v.p)->op);
    dump_node(((strm_node_op*) node->value.v.p)->rhs, indent+1);
    break;
  case STRM_NODE_BLOCK:
    printf("BLOCK:\n");
    dump_node(((strm_node_block*) node->value.v.p)->args, indent+1);
    dump_node(((strm_node_block*) node->value.v.p)->compstmt, indent+1);
    break;
  case STRM_NODE_CALL:
    printf("CALL:\n");
    dump_node(((strm_node_call*) node->value.v.p)->cond, indent+2);
    dump_node(((strm_node_call*) node->value.v.p)->ident, indent+2);
    dump_node(((strm_node_call*) node->value.v.p)->args, indent+2);
    dump_node(((strm_node_call*) node->value.v.p)->blk, indent+2);
    break;
  case STRM_NODE_IDENT:
    printf("IDENT: %d\n", node->value.v.id);
    break;
  case STRM_NODE_VALUE:
    switch (node->value.t) {
    case STRM_VALUE_DOUBLE:
      printf("VALUE(NUMBER): %f\n", node->value.v.d);
      break;
    case STRM_VALUE_STRING:
      printf("VALUE(STRING): %s\n", node->value.v.s);
      break;
    case STRM_VALUE_BOOL:
      printf("VALUE(BOOL): %s\n", node->value.v.i ? "true" : "false");
      break;
    case STRM_VALUE_NIL:
      printf("VALUE(NIL): nil\n");
      break;
    case STRM_VALUE_ARRAY:
      printf("VALUE(ARRAY):\n");
      {
        strm_array* arr0 = node->value.v.p;
        for (i = 0; i < arr0->len; i++)
          dump_node(arr0->data[i], indent+1);
      }
      break;
    case STRM_VALUE_MAP:
      printf("VALUE(MAP):\n");
      {
        strm_array* arr0 = node->value.v.p;
        for (i = 0; i < arr0->len; i++) {
          strm_node* pair = arr0->data[i];
		  strm_pair* pair0 = pair->value.v.p;
          dump_node(pair0->key, indent+1);
          dump_node(pair0->value, indent+1);
        }
      }
      break;
    default:
      printf("VALUE(UNKNOWN): %p\n", node->value.v.p);
      break;
    }
    break;
  default:
    printf("UNKNWON(%d)\n", node->type);
    break;
  }
}

int
main(int argc, const char**argv)
{
  int i, n = 0;
  parser_state state;

  strm_parse_init(&state);

  // yydebug = 1;
  if (argc == 1) {              /* no args */
    n = strm_parse_input(&state, stdin, "stdin");
  }
  else {
    for (i=1; i<argc; i++) {
      n += strm_parse_file(&state, argv[i]);
    }
  }
  if (n > 0) return 1;
  dump_node(state.lval, 0);
  return 0;
}
