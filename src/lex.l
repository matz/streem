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

if{TRAIL}           LEX_RETURN(keyword_if);
{TRAIL}else{TRAIL}  LEX_RETURN(keyword_else);
break{TRAIL}        LEX_RETURN(keyword_break);
emit{TRAIL}         LEX_RETURN(keyword_emit);
return{TRAIL}       LEX_RETURN(keyword_return);
nil{TRAIL} {
  lval->nd = node_nil();
  LEX_RETURN(keyword_nil);
}
true{TRAIL} {
  lval->nd = node_true();
  LEX_RETURN(keyword_true);
}
false{TRAIL} {
  lval->nd = node_false();
  LEX_RETURN(keyword_false);
}

[A-Za-z_][A-Za-z0-9_]* {
  lval->id = node_ident_of(yytext);
  LEX_RETURN(identifier);
};

{TRAIL}\.{TRAIL} LEX_RETURN('.');
[(\[{,;:]{TRAIL} LEX_RETURN(yytext[0]);
[)\]}]           LEX_RETURN(yytext[0]);
"\n"             LEX_RETURN('\n');
"#"[^\n]*"\n"    LEX_RETURN('\n');

(([1-9][0-9]*)|0)(\.[0-9]*)? {
  double temp;
  sscanf(yytext, "%lf", &temp);
  lval->nd = node_double_new(temp);
  LEX_RETURN(lit_number);
};

\"([^\\\"]|\\.)*\"      {
  lval->nd = node_string_len_new(yytext+1, strlen(yytext)-2);
  LEX_RETURN(lit_string);
};

[ \t] ; 

. { 
    fprintf(stderr, "lexical error ('%s').\n", yytext); 
    exit(1); 
};
%% 
