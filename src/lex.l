/*
** lex.l - streem lexer
**
** See Copyright Notice in LICENSE file.
*/

%{
#include <string.h>
#include "strm.h"
#include "node.h"

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
"->"{TRAIL} LEX_RETURN(op_rasgn);
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
nil                 LEX_RETURN(keyword_nil);
true                LEX_RETURN(keyword_true);
false               LEX_RETURN(keyword_false);

[A-Za-z_][A-Za-z0-9_]* {
  lval->id = node_id(yytext, yyleng);
  LEX_RETURN(identifier);
};

[A-Za-z_][A-Za-z0-9_]*: {
  yytext[yyleng-1]='\0';
  lval->id = node_id(yytext, yyleng-1);
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
  lval->nd = node_double_new(temp);
  LEX_RETURN(lit_number);
};

\"([^\\\"]|\\.)*\"      {
  lval->nd = node_string_new(yytext+1, yyleng-2);
  LEX_RETURN(lit_string);
};

:[0-9a-zA-Z_][0-9a-zA-Z_]* {
  lval->nd = node_string_new(yytext+1, yyleng-1);
  LEX_RETURN(lit_string);
};

\"([^\\\"]|\\.)*\": {
  lval->id = node_id_escaped(yytext+1, yyleng-3);
  LEX_RETURN(label);
};

[ \t] ; 

. { 
    fprintf(stderr, "lexical error ('%s').\n", yytext); 
    exit(1); 
};
%% 
