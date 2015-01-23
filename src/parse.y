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
  strm_node *nd;
  strm_id id;
}

%type <nd> program compstmt
%type <nd> stmts stmt expr condition cond args var primary primary0
%type <nd> opt_args opt_block block terms f_args opt_terms
%type <id> identifier

%pure-parser
%parse-param {parser_state *p}
%lex-param {p}

%{
int yylex(YYSTYPE *yylval, parser_state *p);
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
                ;

compstmt        : stmts opt_terms
                ;

stmts           :
                {
                  $$ = strm_array_new();
                }
                | stmt
                {
                  strm_array_add($$, $1);
                }
                | stmts terms stmt
                {
                  strm_array_add($1, $3);
                }
                | error stmt
                {
                  /* TODO */
                }
                ;

stmt            : var '=' expr
                {
                  $$ = strm_let_new($1, $3);
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
                  $$ = strm_op_new("+", $1, $3);
                }
                | expr op_minus expr
                {
                  $$ = strm_op_new("-", $1, $3);
                }
                | expr op_mult expr
                {
                  $$ = strm_op_new("*", $1, $3);
                }
                | expr op_div expr
                {
                  $$ = strm_op_new("/", $1, $3);
                }
                | expr op_mod expr
                {
                  $$ = strm_op_new("%", $1, $3);
                }
                | expr op_bar expr
                {
                  $$ = strm_op_new("|", $1, $3);
                }
                | expr op_amper expr
                {
                  $$ = strm_op_new("&", $1, $3);
                }
                | expr op_gt expr
                {
                  $$ = strm_op_new("<", $1, $3);
                }
                | expr op_ge expr
                {
                  $$ = strm_op_new("<=", $1, $3);
                }
                | expr op_lt expr
                {
                  $$ = strm_op_new(">", $1, $3);
                }
                | expr op_le expr
                {
                  $$ = strm_op_new(">=", $1, $3);
                }
                | expr op_eq expr
                {
                  $$ = strm_op_new("==", $1, $3);
                }
                | expr op_neq expr
                {
                  $$ = strm_op_new("!=", $1, $3);
                }
                | op_plus expr                 %prec '!'
                {
                  $$ = strm_value_new($2);
                }
                | op_minus expr                %prec '!'
                {
                  $$ = strm_value_new($2);
                }
                | '!' expr
                {
                  $$ = strm_op_new("!", NULL, $2);
                }
                | '~' expr
                {
                  $$ = strm_op_new("~", NULL, $2);
                }
                | expr op_and expr
                {
                  $$ = strm_op_new("&&", $1, $3);
                }
                | expr op_or expr
                {
                  $$ = strm_op_new("||", $1, $3);
                }
                | primary
                {
                  $$ = $1;
                }
                ;

condition       : condition op_plus condition
                {
                  $$ = strm_op_new("+", $1, $3);
                }
                | condition op_minus condition
                {
                  $$ = strm_op_new("-", $1, $3);
                }
                | condition op_mult condition
                {
                  $$ = strm_op_new("*", $1, $3);
                }
                | condition op_div condition
                {
                  $$ = strm_op_new("/", $1, $3);
                }
                | condition op_mod condition
                {
                  $$ = strm_op_new("%", $1, $3);
                }
                | condition op_bar condition
                {
                  $$ = strm_op_new("|", $1, $3);
                }
                | condition op_amper condition
                {
                  $$ = strm_op_new("&", $1, $3);
                }
                | condition op_gt condition
                {
                  $$ = strm_op_new("<", $1, $3);
                }
                | condition op_ge condition
                {
                  $$ = strm_op_new("<=", $1, $3);
                }
                | condition op_lt condition
                {
                  $$ = strm_op_new(">", $1, $3);
                }
                | condition op_le condition
                {
                  $$ = strm_op_new(">=", $1, $3);
                }
                | condition op_eq condition
                {
                  $$ = strm_op_new("==", $1, $3);
                }
                | condition op_neq condition
                {
                  $$ = strm_op_new("!=", $1, $3);
                }
                | op_plus condition            %prec '!'
                {
                  $$ = strm_value_new($2);
                }
                | op_minus condition           %prec '!'
                {
                  $$ = strm_value_new($2);
                }
                | '!' condition
                {
                  $$ = strm_op_new("!", NULL, $2);
                }
                | '~' condition
                {
                  $$ = strm_op_new("~", NULL, $2);
                }
                | condition op_and condition
                {
                  $$ = strm_op_new("&&", $1, $3);
                }
                | condition op_or condition
                {
                  $$ = strm_op_new("||", $1, $3);
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
                  $$ = strm_array_new();
                }
                | args
                {
                  $$ = $1;
                }
                ;

args            : expr
                {
                  $$ = strm_array_new();
                }
                | args ',' expr
                {
                  strm_array_add($1, $3);
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
                  $$ = strm_ident_new($1);
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
                  $$ = strm_funcall_new($1, $3, NULL);
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
                  $$ = strm_funcall_new($1, $3, $5);
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
                  $$ = strm_nil_value();
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
                  $$ = strm_array_new();
                }
                | f_args ',' identifier
                {
                  strm_array_add($$, $1);
                }
                ;

opt_terms       : /* none */
                {
                  $$ = strm_array_new();
                }
                | terms
                {
                  strm_array_add($$, $1);
                }
                ;

terms           : term
                {
                  $$ = strm_array_new();
                }
                | terms term {yyerrok;}
                {
                  strm_array_add($$, $1);
                }
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

static int
syntax_check(FILE *f, const char *fname)
{
  parser_state state = {0, NULL, fname, 1, 1};
  int n;

  yyin = f;
  n = yyparse(&state);

  printf("%p\n", state.lval);
  if (n == 0 && state.nerr == 0) {
    printf("%s: Syntax OK\n", fname);
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
