#include "strm.h"
#include "node.h"
#include <stdio.h>

static void
print_id(const char* pre, node_id id)
{
  strm_string *str = id;

  fputs(pre, stdout);
  fprintf(stdout, "%*s\n", str->len, str->ptr);
}

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
      node_values* arr0 = (node_values*)np;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
    break;
  case NODE_IF:
    {
      printf("IF:\n");
      dump_node(((node_if*)np)->cond, indent+1);
      for (i = 0; i < indent; i++)
        putchar(' ');
      printf("THEN:\n");
      dump_node(((node_if*)np)->compstmt, indent+1);
      node* opt_else = ((node_if*)np)->opt_else;
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
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    print_id("op: ", ((node_op*) np)->op);
    dump_node(((node_op*) np)->lhs, indent+1);
    dump_node(((node_op*) np)->rhs, indent+1);
    break;
  case NODE_BLOCK:
    printf("BLOCK:\n");
    dump_node(((node_block*) np)->args, indent+1);
    dump_node(((node_block*) np)->compstmt, indent+1);
    break;
  case NODE_CALL:
    printf("CALL:\n");
    dump_node(((node_call*) np)->recv, indent+2);
    dump_node(((node_call*) np)->ident, indent+2);
    dump_node(((node_call*) np)->args, indent+2);
    dump_node(((node_call*) np)->blk, indent+2);
    break;
  case NODE_RETURN:
    printf("RETURN:\n");
    dump_node(((node_return*) np)->rv, indent+1);
    break;
  case NODE_IDENT:
    print_id("IDENT: ", np->value.v.id);
    break;

  case NODE_ARRAY:
    printf("ARRAY:\n");
    {
      node_values* arr0 = (node_values*)np;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
    break;
  case NODE_MAP:
    printf("MAP:\n");
    {
      node_values* arr0 = (node_values*)np;
      for (i = 0; i < arr0->len; i++) {
        node* pair = arr0->data[i];
        node_pair* pair0 = (node_pair*)pair;
        dump_node(pair0->key, indent+1);
        dump_node(pair0->value, indent+1);
      }
    }
    break;
  case NODE_STMTS:
    printf("STMTS:\n");
    {
      node_values* arr0 = (node_values*)np;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
    break;

  case NODE_VALUE:
    switch (np->value.t) {
    case NODE_VALUE_INT:
      printf("VALUE(NUMBER): %ld\n", np->value.v.i);
      break;
    case NODE_VALUE_DOUBLE:
      printf("VALUE(NUMBER): %f\n", np->value.v.d);
      break;
    case NODE_VALUE_STRING:
      printf("VALUE(STRING): \"%*s\"\n", np->value.v.s->len, np->value.v.s->ptr);
      break;
    case NODE_VALUE_BOOL:
      printf("VALUE(BOOL): %s\n", np->value.v.i ? "true" : "false");
      break;
    case NODE_VALUE_NIL:
      printf("VALUE(NIL): nil\n");
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
  const char *prog = argv[0];
  const char *e_prog = NULL;
  int i, n = 0, verbose = FALSE, check = FALSE;
  parser_state state;

  while (argc > 1 && argv[1][0] == '-') {
    const char *s = argv[1]+1;
    while (*s) {
      switch (*s) {
      case 'v':
        verbose = TRUE;
        break;
      case 'c':
        check = TRUE;
        break;
      case 'e':
        if (s[1] == '\0') {
          e_prog = argv[2];
          argc--; argv++;
        }
        else {
          e_prog = &s[1];
        }
        goto next_arg;
      default:
        fprintf(stderr, "%s: unknown option -%c\n", prog, *s);
      }
      s++;
    }
  next_arg:
    argc--; argv++;
  }
  node_parse_init(&state);

  if (e_prog) {
    n += node_parse_string(&state, e_prog);
  }
  else if (argc == 1) {              /* no args */
    n = node_parse_input(&state, stdin, "stdin");
  }
  else {
    for (i=1; i<argc; i++) {
      n += node_parse_file(&state, argv[i]);
    }
  }

  if (n == 0) {
    if (verbose) {
      dump_node(state.lval, 0);
    }
    if (check) {
      puts("Syntax OK");
    }
    else {
      node_run(&state);
      strm_loop();
    }
  }
  else if (check) {
    puts("Syntax NG");
  }
  node_parse_free(&state);
  return n > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
