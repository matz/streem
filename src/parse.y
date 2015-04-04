/*
** parse.y - streem parser
**
** See Copyright Notice in LICENSE file.
*/

%{
#define YYDEBUG 1
#define YYERROR_VERBOSE 1

#include "strm.h"
#include "node.h"
%}

%union {
  node* nd;
  node_id id;
}

%type <nd> program compstmt
%type <nd> stmt expr condition block cond var primary primary0
%type <nd> stmts args opt_args opt_block f_args map map_args bparam
%type <nd> opt_else opt_elsif
%type <id> identifier
%type <nd> lit_string lit_number

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
        op_colon2

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
                      $$ = $1;
                      node_array_add($1, $3);
                    }
                | error stmt
                    {
                    }
                ;

stmt            : var '=' expr
                    {
                      $$ = node_let_new($1, $3);
                    }
                | keyword_emit opt_args
                    {
                      $$ = node_emit_new($2);
                    }
                | keyword_return opt_args
                    {
                      $$ = node_return_new($2);
                    }
                | keyword_break
                    {
                      $$ = node_break_new();
                    }
                | expr
                    {
                      $$ = $1;
                    }
                ;

var             : identifier
                    {
                        $$ = node_ident_new($1);
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
                      $$ = $2;
                    }
                | op_minus expr                %prec '!'
                    {
                      $$ = node_op_new("-", NULL, $2);
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
                      $$ = $2;
                    }
                | op_minus condition           %prec '!'
                    {
                      $$ = node_op_new("-", NULL, $2);
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
                    {
                      $$ = NULL;
                    }
                | opt_elsif keyword_else keyword_if condition '{' compstmt '}'
                    {
                      $$ = node_if_new($4, $6, NULL);
                    }
                ;

opt_else        : opt_elsif
                    {
                      $$ = NULL;
                    }
                | opt_elsif keyword_else '{' compstmt '}'
                    {
                      $$ = $4;
                    }
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
                      node_array_add($$, $1);
                    }
                | args ',' expr
                    {
                      $$ = $1;
                      node_array_add($1, $3);
                    }
                ;

primary0        : lit_number
                | lit_string
                | identifier
                    {
                      $$ = node_ident_new($1);
                    }
                | '(' expr ')'
                    {
                       $$ = $2;
                    }
                | '[' args ']'
                    {
                      $$ = node_array_of($2);
                    }
                | '[' ']'
                    {
                      $$ = node_array_of(NULL);
                    }
                | '[' map_args ']'
                    {
                      $$ = node_map_of($2);
                    }
                | '[' ':' ']'
                    {
                      $$ = node_map_of(NULL);
                    }
                | keyword_if condition '{' compstmt '}' opt_else
                    {
                      $$ = node_if_new($2, $4, $6);
                    }
                | keyword_nil
                    {
                      $$ = node_nil();
                    }
                | keyword_true
                    {
                      $$ = node_true();
                    }
                | keyword_false
                    {
                      $$ = node_false();
                    }
                ;

cond            : primary0
                    {
                       $$ = $1;
                    }
                | identifier '(' opt_args ')'
                    {
                      $$ = node_call_new(NULL, node_ident_new($1), $3, NULL);
                    }
                | cond '.' identifier '(' opt_args ')'
                    {
                      $$ = node_call_new(NULL, node_ident_new($3), $5, NULL);
                    }
                | cond '.' identifier
                    {
                      $$ = node_call_new($1, node_ident_new($3), NULL, NULL);
                    }
                ;

primary         : primary0
                | block
                    {
                      $$ = node_call_new(NULL, NULL, NULL, $1);
                    }
                | identifier block
                    {
                      $$ = node_call_new(NULL, node_ident_new($1), NULL, $2);
                    }
                | identifier '(' opt_args ')' opt_block
                    {
                      $$ = node_call_new(NULL, node_ident_new($1), $3, $5);
                    }
                | primary '.' identifier '(' opt_args ')' opt_block
                    {
                      $$ = node_call_new($1, node_ident_new($3), $5, $7);
                    }
                | primary '.' identifier opt_block
                    {
                      $$ = node_call_new($1, node_ident_new($3), NULL, $4);
                    }
                ;

map             : lit_string ':' expr
                    {
                      $$ = node_pair_new($1, $3);
                    }
                | identifier ':' expr
                    {
                      $$ = node_pair_new(node_ident_str($1), $3);
                    }
                ;

map_args        : map
                    {
                      $$ = node_map_new();
                      node_array_add($$, $1);
                    }
                | map_args ',' map
                    {
                      $$ = $1;
                      node_array_add($$, $3);
                    }
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
                      $$ = node_block_new($2, $3);
                    }
                | '{' compstmt '}'
                    {
                      $$ = node_block_new(NULL, $2);
                    }
                ;

bparam          : op_rasgn
                    {
                      $$ = NULL;
                    }
                | f_args op_rasgn
                    {
                      $$ = $1;
                    }
                ;

f_args          : identifier
                    {
                      $$ = node_array_new();
                      node_array_add($$, node_ident_new($1));
                    }
                | f_args ',' identifier
                    {
                      $$ = $1;
                      node_array_add($$, node_ident_new($3));
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
