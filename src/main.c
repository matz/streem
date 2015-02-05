#include "strm.h"
#include "node.h"
#include <stdio.h>

static void
dump_node(node* np, int indent) {
  int i;
  for (i = 0; i < indent; i++)
    putchar(' ');

  if (!np) {
    printf("NIL\n");
    return;		  
  }

  switch (np->type) {
  case NODE_ARGS:
    {
      node_array* arr0 = np->value.v.p;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
    break;
  case NODE_IF:
    {
      printf("IF:\n");
      dump_node(((node_if*)np->value.v.p)->cond, indent+1);
      for (i = 0; i < indent; i++)
        putchar(' ');
      printf("THEN:\n");
      dump_node(((node_if*)np->value.v.p)->compstmt, indent+1);
      node* opt_else = ((node_if*)np->value.v.p)->opt_else;
      if (opt_else != NULL) {
        for (i = 0; i < indent; i++)
          putchar(' ');
        printf("ELSE:\n");
        dump_node(opt_else, indent+1);
      }
    }
    break;
  case NODE_EMIT:
    printf("EMIT:\n");
    dump_node((node*) np->value.v.p, indent+1);
    break;
  case NODE_OP:
    printf("OP:\n");
    dump_node(((node_op*) np->value.v.p)->lhs, indent+1);
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    puts(((node_op*) np->value.v.p)->op);
    dump_node(((node_op*) np->value.v.p)->rhs, indent+1);
    break;
  case NODE_BLOCK:
    printf("BLOCK:\n");
    dump_node(((node_block*) np->value.v.p)->args, indent+1);
    dump_node(((node_block*) np->value.v.p)->compstmt, indent+1);
    break;
  case NODE_CALL:
    printf("CALL:\n");
    dump_node(((node_call*) np->value.v.p)->cond, indent+2);
    dump_node(((node_call*) np->value.v.p)->ident, indent+2);
    dump_node(((node_call*) np->value.v.p)->args, indent+2);
    dump_node(((node_call*) np->value.v.p)->blk, indent+2);
    break;
  case NODE_IDENT:
    printf("IDENT: %p\n", np->value.v.id);
    break;
  case NODE_VALUE:
    switch (np->value.t) {
    case STRM_VALUE_DOUBLE:
      printf("VALUE(NUMBER): %f\n", np->value.v.d);
      break;
    case STRM_VALUE_STRING:
      printf("VALUE(STRING): %s\n", np->value.v.s);
      break;
    case STRM_VALUE_BOOL:
      printf("VALUE(BOOL): %s\n", np->value.v.i ? "true" : "false");
      break;
    case STRM_VALUE_NIL:
      printf("VALUE(NIL): nil\n");
      break;
    case STRM_VALUE_ARRAY:
      printf("VALUE(ARRAY):\n");
      {
        node_array* arr0 = np->value.v.p;
        for (i = 0; i < arr0->len; i++)
          dump_node(arr0->data[i], indent+1);
      }
      break;
    case STRM_VALUE_MAP:
      printf("VALUE(MAP):\n");
      {
        node_array* arr0 = np->value.v.p;
        for (i = 0; i < arr0->len; i++) {
          node* pair = arr0->data[i];
		  node_pair* pair0 = pair->value.v.p;
          dump_node(pair0->key, indent+1);
          dump_node(pair0->value, indent+1);
        }
      }
      break;
    default:
      printf("VALUE(UNKNOWN): %p\n", np->value.v.p);
      break;
    }
    break;
  default:
    printf("UNKNWON(%d)\n", np->type);
    break;
  }
}

int
main(int argc, const char**argv)
{
  int i, n = 0;
  parser_state state;

  strm_parse_init(&state);

  if (argc == 1) {              /* no args */
    n = strm_parse_input(&state, stdin, "stdin");
  }
  else {
    for (i=1; i<argc; i++) {
      n += strm_parse_file(&state, argv[i]);
    }
  }
  if (n == 0)
    dump_node(state.lval, 0);
  strm_parse_free(&state);
  return n > 0 ? 1 : 0;
}
