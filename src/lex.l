/*
** lex.l - streem lexer
**
** See Copyright Notice in LICENSE file.
*/

%{
#include <string.h>
#include <ctype.h>
#include "strm.h"

#define YY_DECL    int yylex(YYSTYPE *lval, parser_state *p)

static int
lex_return(parser_state *p, int c)
{
  p->lineno = p->tline;
  p->tline = yylineno;
  return c;
}

#define LEX_RETURN(c) return lex_return(p, c)
%}

%option noyywrap
%option yylineno

TRAIL  ([\t \n]|"#"[^\n]*"\n")*
CHAR   [a-zA-Z_]|[\302-\337][\200-\277]|[\340-\357][\200-\277][\200-\277]|[\360-\367][\200-\277][\200-\277]|[\370-\373][\200-\277][\200-\277][\200-\277][\200-\277]|[\374-\375][\200-\277][\200-\277][\200-\277][\200-\277][\200-\277]
CHNUM  ({CHAR}|[0-9])
WORD   {CHAR}{CHNUM}*
DATE   [0-9]+\.[0-9]+\.[0-9]+
TIME   [0-9]+":"[0-9]+(":"[0-9]+)?(\.[0-9]+)?
TZONE  "Z"|[+-][0-9]+(":"[0-9]+)?
%%
"+"{TRAIL}  LEX_RETURN(op_plus);
"-"{TRAIL}  LEX_RETURN(op_minus);
"*"{TRAIL}  LEX_RETURN(op_mult);
"/"{TRAIL}  LEX_RETURN(op_div);
"%"{TRAIL}  LEX_RETURN(op_mod);
"=="{TRAIL} LEX_RETURN(op_eq);
"!="{TRAIL} LEX_RETURN(op_neq);
"<"{TRAIL}  LEX_RETURN(op_lt);
"<="{TRAIL} LEX_RETURN(op_le);
">"{TRAIL}  LEX_RETURN(op_gt);
">="{TRAIL} LEX_RETURN(op_ge);
"&&"{TRAIL} LEX_RETURN(op_and);
"||"{TRAIL} LEX_RETURN(op_or);
"|"{TRAIL}  LEX_RETURN(op_bar);
"&"{TRAIL}  LEX_RETURN(op_amper);
"<-"{TRAIL} LEX_RETURN(op_lasgn);
"=>"{TRAIL} LEX_RETURN(op_rasgn);
"->"{TRAIL} LEX_RETURN(op_lambda);
")"" "*"->"{TRAIL} LEX_RETURN(op_lambda2);
"="{TRAIL}  LEX_RETURN('=');
"::"{TRAIL} LEX_RETURN(op_colon2);

if                  LEX_RETURN(keyword_if);
{TRAIL}else{TRAIL}  LEX_RETURN(keyword_else);
break{TRAIL}        LEX_RETURN(keyword_break);
emit                LEX_RETURN(keyword_emit);
return              LEX_RETURN(keyword_return);
namespace           LEX_RETURN(keyword_namespace);
class               LEX_RETURN(keyword_class);
import              LEX_RETURN(keyword_import);
def                 LEX_RETURN(keyword_def);
method              LEX_RETURN(keyword_method);
new                 LEX_RETURN(keyword_new);
nil                 LEX_RETURN(keyword_nil);
true                LEX_RETURN(keyword_true);
false               LEX_RETURN(keyword_false);

{WORD} {
  lval->id = node_str_new(yytext, yyleng);
  LEX_RETURN(identifier);
};

{WORD}: {
  yytext[yyleng-1]='\0';
  lval->id = node_str_new(yytext, yyleng-1);
  LEX_RETURN(label);
};

{TRAIL}\.{TRAIL} LEX_RETURN('.');
[(\[{,;:]{TRAIL} LEX_RETURN(yytext[0]);
[)\]}]           LEX_RETURN(yytext[0]);
"\n"             LEX_RETURN('\n');
"#"[^\n]*"\n"    LEX_RETURN('\n');

(([1-9][0-9]*)|0) {
  lval->nd = node_int_new(atol(yytext));
  LEX_RETURN(lit_number);
};

(([1-9][0-9]*)|0)(\.[0-9][0-9]*)? {
  double temp;
  sscanf(yytext, "%lf", &temp);
  lval->nd = node_float_new(temp);
  LEX_RETURN(lit_number);
};

0x[0-9a-fA-F]+ {
  unsigned long temp;
  sscanf(yytext+2, "%lx", &temp);
  lval->nd = node_int_new(temp);
  LEX_RETURN(lit_number);
};

0o[0-7]+ {
  unsigned long temp;
  sscanf(yytext+2, "%lo", &temp);
  lval->nd = node_int_new(temp);
  LEX_RETURN(lit_number);
};

{DATE}("T"{TIME}{TZONE}?)? {
  lval->nd = node_time_new(yytext, yyleng);
  if (lval->nd == NULL) yyerror(p, "bad time format");
  LEX_RETURN(lit_string);
};

\"([^\\\"]|\\.)*\"      {
  lval->nd = node_string_new(yytext+1, yyleng-2);
  LEX_RETURN(lit_string);
};

:{WORD} {
  lval->nd = node_string_new(yytext+1, yyleng-1);
  LEX_RETURN(lit_string);
};

\"([^\\\"]|\\.)*\": {
  lval->id = node_str_escaped(yytext+1, yyleng-3);
  LEX_RETURN(label);
};

[ \t] ;

. {
    char c = yytext[0];

    fprintf(stderr, "%s:%d:lexical error", p->fname, yylineno);
    if ((c & 0x80) || !isprint(c)) {
      fprintf(stderr, "('\\%03o').\n", c);
    }
    else if (c == '\\') {
      fprintf(stderr, "('\\\\').\n");
    }
    else {
      fprintf(stderr, "('%c').\n", c);
    }
    exit(1); 
};
%% 
