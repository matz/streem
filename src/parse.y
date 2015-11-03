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
  strm_string* id;
}

%type <nd> program topstmts decls decl_list decl stmts stmt_list
%type <nd> stmt expr condition block cond primary primary0
%type <nd> arg args opt_args opt_block f_args opt_f_args bparam
%type <nd> opt_else opt_elsif
%type <id> identifier var label
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
        keyword_skip
        keyword_return
        keyword_namespace
        keyword_class
        keyword_import
        keyword_def
        keyword_method
        keyword_new
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
        label

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
program         : topstmts
                    { 
                      p->lval = $1;  
                    }
                ;

/* declarations must come before acrtual statements */
topstmts        : decls stmts
                    {
                      $$ = node_stmts_concat($1, $2);
                    }
                ;

decls           : opt_terms
                    {
                      $$ = NULL;
                    }
                | decl_list opt_terms
                    {
                      $$ = $1;
                    }
                ;

decl_list       : decl
                    {
                      $$ = node_stmts_new();
                      if ($1) {
                        node_stmts_add($$, $1);
                      }
                    }
                | decl_list terms decl
                    {
                      if ($3) {
                        node_stmts_add($$, $3);
                      }
                    }
                ;

decl            : /* namespace statement:
                    namespace name_of_namespace {
                      declarations
                    }

                    define a new namespace. */
                  keyword_namespace identifier '{' decls '}'
                    {
                      $$ = node_ns_new($2, $4);
                    }
                | /* class statement:
                    class name_of_namespace {
                      declarations
                    }

                    alias of namespace statement.
                    namespace work like class when bound with new expression. */
                  keyword_class identifier '{' decls '}'
                    {
                      $$ = node_ns_new($2, $4);
                    }
                | /* import statement:
                     import name_of_namespace

                     copies names to the current namespace. */
                  keyword_import identifier
                    {
                      $$ = node_import_new($2);
                    }
                | /* def statement:
                    def foo(args) {
                      statements
                    }

                    define a function named foo. */
                  keyword_def identifier '(' opt_f_args ')' '{' stmts '}'
                    {
                      $$ = node_let_new($2, node_lambda_new($4, $7));
                    }
                | /* method statement:
                    method foo(args) {
                      statements
                    }

                    define a method named foo.
                    Above method statement works like:

                    def method foo(self, args) {
                      statements
                    } */
                  keyword_method identifier '(' opt_f_args ')' '{' stmts '}'
                    {
                      $$ = node_let_new($2, node_method_new($4, $7));
                    }
                ;

stmts           : opt_terms
                    {
                      $$ = NULL;
                    }
                | stmt_list opt_terms
                    {
                      $$ = $1;
                    }
                ;

stmt_list       : stmt
                    {
                      $$ = node_stmts_new();
                      if ($1) {
                        node_stmts_add($$, $1);
                      }
                    }
                | stmt_list terms stmt
                    {
                      if ($3) {
                        node_stmts_add($$, $3);
                      }
                    }
                ;

stmt            : var '=' expr
                    {
                      $$ = node_let_new($1, $3);
                    }
                | keyword_skip
                    {
                      $$ = node_skip_new();
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
                ;

var             : identifier
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
                | opt_elsif keyword_else keyword_if condition '{' stmts '}'
                    {
                      if ($1)
                        ((node_if*)$1)->opt_else = node_if_new($4, $6, NULL);
                      else
                        $$ = node_if_new($4, $6, NULL);
                    }
                ;

opt_else        : opt_elsif
                | opt_elsif keyword_else '{' stmts '}'
                    {
                      if ($1) {
                        node_if* n = (node_if*)$1;

                        while (n->opt_else && n->opt_else->type == NODE_IF) {
                          n = (node_if*)n->opt_else;
                        }
                        n->opt_else = $4;
                      }
                      else
                        $$ = $4;
                    }
                ;

opt_args        : /* none */
                    {
                      $$ = node_array_new();
                    }
                | args
                    {
                      $$ = node_array_headers($1);
                    }
                ;


arg             : expr
                | label expr
                    {
                      $$ = node_pair_new($1, $2);
                    }
                ;


args            : arg
                    {
                      $$ = node_array_new();
                      node_array_add($$, $1);
                    }
                | args ',' arg
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
                      $$ = node_array_headers($2);
                    }
                | '[' ']'
                    {
                      $$ = node_array_new();
                    }
                | keyword_if condition '{' stmts '}' opt_else
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
                | keyword_new identifier '(' opt_args ')'
                    {
                      $$ = NULL;
                    }
                | identifier '(' opt_args ')'
                    {
                      $$ = node_call_new(node_id_str($1), NULL, $3, NULL);
                    }
                | cond '.' identifier '(' opt_args ')'
                    {
                      $$ = node_call_new(node_id_str($3), NULL, $5, NULL);
                    }
                | cond '.' identifier
                    {
                      $$ = node_call_new(node_id_str($3), $1, NULL, NULL);
                    }
                ;

primary         : primary0
                | block
                | keyword_new identifier '(' opt_args ')' opt_block
                    {
                      $$ = NULL;
                    }
                | identifier block
                    {
                      $$ = node_call_new(node_id_str($1), NULL, NULL, $2);
                    }
                | identifier '(' opt_args ')' opt_block
                    {
                      $$ = node_call_new(node_id_str($1), NULL, $3, $5);
                    }
                | primary '.' identifier '(' opt_args ')' opt_block
                    {
                      $$ = node_call_new(node_id_str($3), $1, $5, $7);
                    }
                | primary '.' identifier opt_block
                    {
                      $$ = node_call_new(node_id_str($3), $1, NULL, $4);
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

block           : '{' bparam stmts '}'
                    {
                      $$ = node_lambda_new($2, $3);
                    }
                | '{' stmts '}'
                    {
                      $$ = node_lambda_new(NULL, $2);
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

opt_f_args      : /* no args */
                    {
                      $$ = NULL;
                    }
                | f_args
                ;

f_args          : identifier
                    {
                      $$ = node_args_new();
                      node_args_add($$, node_id_str($1));
                    }
                | f_args ',' identifier
                    {
                      $$ = $1;
                      node_args_add($$, node_id_str($3));
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
    fprintf(stderr, "%d:%s\n", p->lineno, s);
  }
}
