/*
** parse.y - streem parser
**
** See Copyright Notice in LICENSE file.
*/

%{
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "strm.h"
%}

%union {
  strm_node* nd;
  strm_id id;
}

%type <nd> program compstmt
%type <nd> stmt expr condition block cond var primary primary0
%type <nd> stmts args opt_args opt_block f_args
%type <id> identifier

%pure-parser
%parse-param {parser_state *p}
%lex-param {p}

%{
int yylex(YYSTYPE *lval, parser_state *p);
static void yyerror(parser_state *p, const char *s);
%}

%token
        keyword_if
        keyword_else
        keyword_do
        keyword_break
        keyword_emit
        keyword_return
        keyword_nil
        keyword_true
        keyword_false
        op_lasgn
        op_rasgn
        op_plus
        op_minus
        op_mult
        op_div
        op_mod
        op_eq
        op_neq
        op_lt
        op_le
        op_gt
        op_ge
        op_and
        op_or
        op_bar
        op_amper

%token
        lit_number
        lit_string
        identifier

/*
 * precedence table
 */

%nonassoc op_LOWEST

%left  op_amper
%left  op_bar
%left  op_or
%left  op_and
%nonassoc  op_eq op_neq
%left  op_lt op_le op_gt op_ge
%left  op_plus op_minus
%left  op_mult op_div op_mod
%right '!' '~'

%token op_HIGHEST

%%
program         : compstmt
                    { 
                      p->lval = $1;  
                    }
                ;

compstmt        : stmts opt_terms
                ;

stmts           :
                    {
                      $$ = node_array_new();
                    }
                | stmt
                    {
                      $$ = node_array_new();
                      node_array_add($$, $1);
                    }
                | stmts terms stmt
                    {
                      node_array_add($1, $3);
                    }
                | error stmt
                    {
                      /* TODO */
                    }
                ;

stmt            : var '=' expr
                    {
                      $$ = node_let_new($1, $3);
                    }
                | keyword_emit opt_args
                    {
                       /* TODO */
                    }
                | keyword_return opt_args
                    {
                      /* TODO */
                    }
                | keyword_break
                    {
                      /* TODO */
                    }
                | expr
                    {
                      $$ = $1;
                    }
                ;

var             : identifier
                    {
                    }
                ;

expr            : expr op_plus expr
                    {
                      $$ = node_op_new("+", $1, $3);
                    }
                | expr op_minus expr
                    {
                      $$ = node_op_new("-", $1, $3);
                    }
                | expr op_mult expr
                    {
                      $$ = node_op_new("*", $1, $3);
                    }
                | expr op_div expr
                    {
                      $$ = node_op_new("/", $1, $3);
                    }
                | expr op_mod expr
                    {
                      $$ = node_op_new("%", $1, $3);
                    }
                | expr op_bar expr
                    {
                      $$ = node_op_new("|", $1, $3);
                    }
                | expr op_amper expr
                    {
                      $$ = node_op_new("&", $1, $3);
                    }
                | expr op_gt expr
                    {
                      $$ = node_op_new(">", $1, $3);
                    }
                | expr op_ge expr
                    {
                      $$ = node_op_new(">=", $1, $3);
                    }
                | expr op_lt expr
                    {
                      $$ = node_op_new("<", $1, $3);
                    }
                | expr op_le expr
                    {
                      $$ = node_op_new("<=", $1, $3);
                    }
                | expr op_eq expr
                    {
                      $$ = node_op_new("==", $1, $3);
                    }
                | expr op_neq expr
                    {
                      $$ = node_op_new("!=", $1, $3);
                    }
                | op_plus expr                 %prec '!'
                    {
                      $$ = node_value_new($2);
                    }
                | op_minus expr                %prec '!'
                    {
                      $$ = node_value_new($2);
                    }
                | '!' expr
                    {
                      $$ = node_op_new("!", NULL, $2);
                    }
                | '~' expr
                    {
                      $$ = node_op_new("~", NULL, $2);
                    }
                | expr op_and expr
                    {
                      $$ = node_op_new("&&", $1, $3);
                    }
                | expr op_or expr
                    {
                      $$ = node_op_new("||", $1, $3);
                    }
                | primary
                    {
                      $$ = $1;
                    }
                ;

condition       : condition op_plus condition
                    {
                      $$ = node_op_new("+", $1, $3);
                    }
                | condition op_minus condition
                    {
                      $$ = node_op_new("-", $1, $3);
                    }
                | condition op_mult condition
                    {
                      $$ = node_op_new("*", $1, $3);
                    }
                | condition op_div condition
                    {
                      $$ = node_op_new("/", $1, $3);
                    }
                | condition op_mod condition
                    {
                      $$ = node_op_new("%", $1, $3);
                    }
                | condition op_bar condition
                    {
                      $$ = node_op_new("|", $1, $3);
                    }
                | condition op_amper condition
                    {
                      $$ = node_op_new("&", $1, $3);
                    }
                | condition op_gt condition
                    {
                      $$ = node_op_new(">", $1, $3);
                    }
                | condition op_ge condition
                    {
                      $$ = node_op_new(">=", $1, $3);
                    }
                | condition op_lt condition
                    {
                      $$ = node_op_new("<", $1, $3);
                    }
                | condition op_le condition
                    {
                      $$ = node_op_new("<=", $1, $3);
                    }
                | condition op_eq condition
                    {
                      $$ = node_op_new("==", $1, $3);
                    }
                | condition op_neq condition
                    {
                      $$ = node_op_new("!=", $1, $3);
                    }
                | op_plus condition            %prec '!'
                    {
                      $$ = node_value_new($2);
                    }
                | op_minus condition           %prec '!'
                    {
                      $$ = node_value_new($2);
                    }
                | '!' condition
                    {
                      $$ = node_op_new("!", NULL, $2);
                    }
                | '~' condition
                    {
                      $$ = node_op_new("~", NULL, $2);
                    }
                | condition op_and condition
                    {
                      $$ = node_op_new("&&", $1, $3);
                    }
                | condition op_or condition
                    {
                      $$ = node_op_new("||", $1, $3);
                    }
                | cond
                    {
                      $$ = $1;
                    }
                ;

opt_elsif       : /* none */
                | opt_elsif keyword_else keyword_if condition '{' compstmt '}'
                ;

opt_else        : opt_elsif
                | opt_elsif keyword_else '{' compstmt '}'
                ;

opt_args        : /* none */
                    {
                      $$ = node_array_new();
                    }
                | args
                    {
                      $$ = $1;
                    }
                ;

args            : expr
                    {
                      $$ = node_array_new();
                    }
                | args ',' expr
                    {
                      node_array_add($1, $3);
                    }
                ;

primary0        : lit_number
                    {
                      /* TODO */
                    }
                | lit_string
                    {
                      /* TODO */
                    }
                | identifier
                    {
                      $$ = node_ident_new($1);
                    }
                | '(' expr ')'
                    {
                      /* TODO */
                    }
                | '[' args ']'
                    {
                      /* TODO */
                    }
                | '[' ']'
                    {
                      /* TODO */
                    }
                | '[' map_args ']'
                    {
                      /* TODO */
                    }
                | '[' ':' '}'
                    {
                      /* TODO */
                    }
                | keyword_if condition '{' compstmt '}' opt_else
                    {
                      /* TODO */
                    }
                | keyword_nil
                    {
                      /* TODO */
                    }
                | keyword_true
                    {
                      /* TODO */
                    }
                | keyword_false
                    {
                      /* TODO */
                    }
                ;

cond            : primary0
                    {
                       $$ = $1;
                    }
                | identifier '(' opt_args ')'
                    {
                      $$ = node_funcall_new($1, $3, NULL);
                    }
                | cond '.' identifier '(' opt_args ')'
                | cond '.' identifier
                ;

primary         : primary0
                    {
                       $$ = $1;
                    }
                | block
                    {
                    }
                | identifier block
                    {
                    }
                | identifier '(' opt_args ')' opt_block
                    {
                      $$ = node_funcall_new($1, $3, $5);
                    }
                | primary '.' identifier '(' opt_args ')' opt_block
                | primary '.' identifier opt_block
                ;

map             : lit_string ':' expr
                | identifier ':' expr
                ;

map_args        : map
                | map_args ',' map
                ;

opt_block       : /* none */
                    {
                      $$ = NULL;
                    }
                | block
                    {
                       $$ = $1;
                    }
                ;

block           : '{' bparam compstmt '}'
                    {
                      /* TODO */
                    }
                | '{' compstmt '}'
                    {
                      /* TODO */
                    }
                ;

bparam          : op_rasgn
                    {
                      /* TODO */
                    }
                | f_args op_rasgn
                    {
                      /* TODO */
                    }
                ;

f_args          : identifier
                    {
                      $$ = node_array_new();
                    }
                | f_args ',' identifier
                    {
                      node_array_add($$, $1);
                    }
                ;

opt_terms       : /* none */
                | terms
                ;

terms           : term
                | terms term {yyerrok;}
                ;

term            : ';' {yyerrok;}
                | '\n'
                ;
%%
//#define yylval  (*((YYSTYPE*)(p->lval)))

#include "lex.yy.c"

static void
yyerror(parser_state *p, const char *s)
{
  p->nerr++;
  if (p->fname) {
    fprintf(stderr, "%s:%d:%s\n", p->fname, p->lineno, s);
  }
  else {
    fprintf(stderr, "%s\n", s);
  }
}

static void
dump_node(strm_node* node, int indent) {
  int i;
  if (!node) return;		  
  for (i = 0; i < indent; i++)
    putchar(' ');
  switch (node->type) {
  case STRM_NODE_ARRAY:
    {
      strm_array* arr0 = node->value.v.p;
      for (i = 0; i < arr0->len; i++)
        dump_node(arr0->data[i], indent+1);
    }
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
    }
    break;
  default:
    printf("UNKNWON(%d)\n", node->type);
    break;
  }
}

static int
syntax_check(FILE *f, const char *fname)
{
  parser_state state = {0, NULL, fname, 1, 1};
  int n;

  yyin = f;
  n = yyparse(&state);

  if (n == 0 && state.nerr == 0) {
    printf("%s: Syntax OK\n", fname);
    puts("---------");
    dump_node(state.lval, 0);
    return 0;
  }
  else {
    printf("%s: Syntax NG\n", fname);
    return 1;
  }
}

static int
syntax_check_file(const char* fname)
{
  int n;
  FILE *f = fopen(fname, "r");

  if (f == NULL) {
    fprintf(stderr, "failed to open file: %s\n", fname);
    return 1;
  }
  n = syntax_check(f, fname);
  fclose(f);
  return n;
}

int
main(int argc, const char**argv)
{
  int i, n = 0;

  // yydebug = 1;
  if (argc == 1) {              /* no args */
    n = syntax_check(stdin, "stdin");
  }
  else {
    for (i=1; i<argc; i++) {
      n += syntax_check_file(argv[i]);
    }
  }

  if (n > 0) return 1;
  return 0;
}
