#include "strm.h"
#include "node.h"
#include <stdio.h>

#define STREEM_EXT ".strm"

static int
ends_with(const char* str, const char* suffix) {
  size_t suffix_len = strlen(suffix);
  size_t str_len = strlen(str);
  if (suffix_len > str_len) {
    return FALSE;
  }

  return !strcmp(&str[str_len-suffix_len], suffix);
}

static void
fprint_str(node_string str, FILE *f)
{
  fprintf(f, "%.*s", (int)str->len, str->buf);
}

static void
print_str(node_string name)
{
  fprint_str(name, stdout);
  fputs("\n", stdout);
}

static void
print_id(const char* pre, node_string name)
{
  fputs(pre, stdout);
  print_str(name);
}

static void
print_quoted_id(const char* pre, node_string name)
{
  fputs(pre, stdout);
  fputs("\"", stdout);
  fprint_str(name, stdout);
  fputs("\"\n", stdout);
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
      node_args* args = (node_args*)np;

      printf("ARGS(%d):\n", args->len);
      for (i = 0; i < args->len; i++) {
        int j;
        node_string s = args->data[i];
        for (j = 0; j < indent+1; j++)
          putchar(' ');
        print_str(s);
      }
    }
    break;
  case NODE_IF:
    {
      printf("IF:\n");
      dump_node(((node_if*)np)->cond, indent+1);
      for (i = 0; i < indent; i++)
        putchar(' ');
      printf("THEN:\n");
      dump_node(((node_if*)np)->then, indent+1);
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
    dump_node(((node_emit*)np)->emit, indent+1);
    break;
  case NODE_OP:
    printf("OP:\n");
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    print_id("op: ", ((node_op*) np)->op);
    dump_node(((node_op*)np)->lhs, indent+1);
    dump_node(((node_op*)np)->rhs, indent+1);
    break;
  case NODE_LAMBDA:
    printf("LAMBDA:\n");
    dump_node(((node_lambda*)np)->args, indent+1);
    dump_node(((node_lambda*)np)->body, indent+1);
    break;
  case NODE_PLAMBDA:
    printf("PLAMBDA:\n");
    dump_node(((node_plambda*)np)->pat, indent+1);
    if (((node_plambda*)np)->cond) {
      for (i = 0; i < indent+2; i++)
        putchar(' ');
      printf("IF:\n");
      dump_node(((node_plambda*)np)->cond, indent+2);
    }
    dump_node(((node_plambda*)np)->body, indent+1);
    if (((node_plambda*)np)->next) {
      dump_node(((node_plambda*)np)->next, indent);
    }
    break;
  case NODE_PARRAY:
    printf("PARRAY:\n");
    {
      node_nodes* ary = (node_nodes*)np;
      for (i = 0; i < ary->len; i++)
        dump_node(ary->data[i], indent+1);
    }
    break;
  case NODE_PSTRUCT:
    printf("PSTRUCT:\n");
    {
      node_nodes* ary = (node_nodes*)np;
      for (i = 0; i < ary->len; i++)
        dump_node(ary->data[i], indent+1);
    }
    break;
  case NODE_PSPLAT:
    printf("PSPLAT:\n");
    {
      node_psplat* cons = (node_psplat*)np;
      dump_node(cons->head, indent+1);
      for (i = 0; i < indent+1; i++)
        putchar(' ');
      printf("REST:\n");
      dump_node(cons->mid, indent+2);
      for (i = 0; i < indent+1; i++)
        putchar(' ');
      printf("TAIL:\n");
      dump_node(cons->tail, indent+2);
    }
    break;
  case NODE_SPLAT:
    printf("SPLAT:\n");
    {
      node_splat* splat = (node_splat*)np;
      dump_node(splat->node, indent+1);
    }
    break;
  case NODE_CALL:
    printf("CALL:\n");
    for (i = 0; i < indent+2; i++)
      putchar(' ');
    {
      node_string s = ((node_call*)np)->ident;
      print_str(s);
    }
    dump_node(((node_call*)np)->args, indent+2);
    break;
  case NODE_FCALL:
    printf("FCALL:\n");
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    printf("FUNC:\n");
    dump_node(((node_fcall*)np)->func, indent+2);
    for (i = 0; i < indent+1; i++)
      putchar(' ');
    printf("ARGS:\n");
    dump_node(((node_fcall*)np)->args, indent+2);
    break;
  case NODE_GENFUNC:
    printf("GENFUNC: ");
    {
      node_string s = ((node_genfunc*)np)->id;
      print_str(s);
    }
    break;
  case NODE_RETURN:
    printf("RETURN:\n");
    dump_node(((node_return*)np)->rv, indent+1);
    break;
  case NODE_LET:
    print_id("LET: ", ((node_let*) np)->lhs);
    dump_node(((node_let*) np)->rhs, indent+1);
    break;
  case NODE_IDENT:
    print_id("IDENT: ", ((node_ident*)np)->name);
    break;

  case NODE_ARRAY:
    printf("ARRAY:\n");
    {
      int j;
      node_array* ary = (node_array*)np;

      if (ary->headers) {
        node_string* h = ary->headers;

        for (i = 0; i < ary->len; i++) {
          for (j = 0; j < indent+1; j++)
            putchar(' ');
          print_quoted_id("key: ", h[i]);
          dump_node(ary->data[i], indent+1);
        }
      }
      else {
        for (i = 0; i < ary->len; i++)
          dump_node(ary->data[i], indent+1);
      }
      if (ary->ns) {
        node_string ns = ary->ns;
        for (j = 0; j < indent+1; j++)
          putchar(' ');
        print_quoted_id("class: ", ns);
      }
    }
    break;

  case NODE_NODES:
    printf("NODES:\n");
    {
      node_nodes* ary = (node_nodes*)np;
      for (i = 0; i < ary->len; i++)
        dump_node(ary->data[i], indent+1);
    }
    break;

  case NODE_IMPORT:
    print_id("IMPORT: ", ((node_import*)np)->name);
    break;

  case NODE_NS:
    print_id("NAMESPACE: ", ((node_ns*)np)->name);
    dump_node(((node_ns*) np)->body, indent+1);
    break;

  case NODE_INT:
    printf("VALUE(NUMBER): %d\n", ((node_int*)np)->value);
    break;
  case NODE_FLOAT:
    printf("VALUE(NUMBER): %f\n", ((node_float*)np)->value);
    break;
  case NODE_TIME:
    {
      node_time* nt = (node_time*)np;
      printf("VALUE(TIME): %ld.%ld(%+05d)\n", nt->sec, nt->usec, nt->utc_offset);
    }
    break;
  case NODE_BOOL:
    printf("VALUE(BOOL): %s\n", ((node_bool*)np)->value ? "true" : "false");
    break;
  case NODE_STR:
    print_quoted_id("VALUE(STRING): ", ((node_str*)np)->value);
    break;
  case NODE_NIL:
    printf("VALUE(NIL): nil\n");
    break;
  default:
    printf("UNKNOWN(%d)\n", np->type);
    break;
  }
}

int strm_option_verbose = FALSE;

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
        /* fall through */
      case 'w':
        strm_option_verbose = TRUE;
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
      if (ends_with(argv[i], STREEM_EXT)) {
        n += node_parse_file(&state, argv[i]);        
      }
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
      strm_array av = strm_ary_new(NULL, argc);
      strm_value* buf = strm_ary_ptr(av);
      int i;

      for (i=0; i<argc; i++) {
        buf[i] = strm_str_value(strm_str_new(argv[i], strlen(argv[i])));
      }
      strm_var_def(NULL, "ARGV", strm_ary_value(av));
      node_run(&state);
      strm_loop();
      node_stop();
    }
  }
  else if (check) {
    puts("Syntax NG");
  }
  node_parse_free(&state);
  return n > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
