/*
** parse.y - streem parser
**
** See Copyright Notice in LICENSE file.
*/

%{
typedef struct parser_state {
  int nerr;
  void *lval;
} parser_state;

#define YYDEBUG 1
#define YYERROR_VERBOSE 1
%}

%union {
  double d;
  char *str;
}

%pure-parser
%parse-param {parser_state *p}

%{
int yylex(YYSTYPE*);
static void yyerror(parser_state *p, const char *s);
%}

%token
        keyword_if
        keyword_else
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

stmts           : /* none */
                | stmt
                | stmts terms stmt
                | error stmt
                ;

stmt            : var '=' expr
                | var op_lasgn expr
                | expr op_rasgn var
                | keyword_emit opt_args
                | keyword_return opt_args
                | keyword_break
                | expr
                ;

var             : identifier
                ;

expr            : expr op_plus expr
                | expr op_minus expr
                | expr op_mult expr
                | expr op_div expr
                | expr op_mod expr
                | expr op_bar expr
                | expr op_amper expr
                | expr op_gt expr
                | expr op_ge expr
                | expr op_lt expr
                | expr op_le expr
                | expr op_eq expr
                | expr op_neq expr
                | op_plus expr                 %prec '!'
                | op_minus expr                %prec '!'
                | '!' expr
                | '~' expr
                | expr op_and expr
                | expr op_or expr
                | primary
                ;

opt_elsif       : /* none */
                | opt_elsif keyword_else keyword_if expr '{' compstmt '}'
                ;

opt_else        : opt_elsif
                | opt_elsif keyword_else '{' compstmt '}'
                ;

opt_args        : /* none */
                | args
                ;

args            : expr
                | args ',' expr
                ;

primary         : lit_number
                | lit_string
                | identifier
                | '(' compstmt ')'
                | '[' args ']'
                | '[' ']'
                | '[' map_args ']'
                | '[' ':' ']'
                | block
                | identifier '(' opt_args ')'
                | primary '.' identifier '(' opt_args ')'
                | primary '.' identifier
                | keyword_if expr '{' compstmt '}' opt_else
                | keyword_nil
                | keyword_true
                | keyword_false
                ;

map             : lit_string ':' expr
                | identifier ':' expr
                ;

map_args        : map
                | map_args ',' map
                ;

block           : '{' bparam compstmt '}'
                ;

bparam          : /* none */
                | op_or
                | op_bar op_bar
                | op_bar f_args op_bar
                ;

f_args          : identifier
                | f_args ',' identifier
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
  fprintf(stderr, "%s\n", s);
}

static int
syntax_check(FILE *f, const char *fname)
{
  parser_state state = {0, NULL};
  int n;

  yyin = f;
  n = yyparse(&state);

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
