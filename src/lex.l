/*
** lex.l - streem lexer
**
** See Copyright Notice in LICENSE file.
*/

%{
#include <string.h>

#define YY_DECL    int yylex(YYSTYPE *yylval, parser_state *p)

static char*
strdup0(const char *s)
{
  size_t len = strlen(s);
  char *p;

  p = (char*)malloc(len+1);
  if (p) {
    strcpy(p, s);
  }
  return p;
}

static char*
strndup0(const char *s, size_t n)
{
  size_t i, len;
  const char *p = s;
  char *new;

  for (i=0; i<n && *p; i++,p++)
    ;
  new = (char*)malloc(i+1);
  if (new) {
    memcpy(new, s, i);
    new[i] = '\0';
  }
  return new;
}

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

if{TRAIL}           LEX_RETURN(keyword_if);
{TRAIL}else{TRAIL}  LEX_RETURN(keyword_else);
break{TRAIL}        LEX_RETURN(keyword_break);
emit{TRAIL}         LEX_RETURN(keyword_emit);
return{TRAIL}       LEX_RETURN(keyword_return);
nil{TRAIL}          LEX_RETURN(keyword_nil);
true{TRAIL}         LEX_RETURN(keyword_true);
false{TRAIL}        LEX_RETURN(keyword_false);

[A-Za-z_][A-Za-z0-9_]* {
  yylval->str = strdup0(yytext);
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
  yylval->d = temp;
  LEX_RETURN(lit_number);
};

\"([^\\\"]|\\.)*\"      {
  yylval->str = strndup0(yytext+1, strlen(yytext)-2);
  LEX_RETURN(lit_string);
};

[ \t] ; 

. { 
    fprintf(stderr, "lexical error ('%s').\n", yytext); 
    exit(1); 
};
%% 
