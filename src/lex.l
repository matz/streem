/*
** lex.l - streem lexer
**
** See Copyright Notice in LICENSE file.
*/

%{
#define YY_DECL    int yylex(YYSTYPE *yylval)

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
%}

%option noyywrap

TRAIL  [\t \n]*
%%
"+"{TRAIL}  return op_plus;
"-"{TRAIL}  return op_minus;
"*"{TRAIL}  return op_mult;
"/"{TRAIL}  return op_div;
"%"{TRAIL}  return op_mod;
"=="{TRAIL} return op_eq;
"!="{TRAIL} return op_neq;
"<"{TRAIL}  return op_lt;
"<="{TRAIL} return op_le;
">"{TRAIL}  return op_gt;
">="{TRAIL} return op_ge;
"&&"{TRAIL} return op_and;
"||"{TRAIL} return op_or;
"|"{TRAIL}  return op_bar;
"&"{TRAIL}  return op_amper;
"<-"{TRAIL} return op_lasgn;
"->"{TRAIL} return op_rasgn;
"="{TRAIL}  return '=';

if{TRAIL}           return keyword_if;
{TRAIL}else{TRAIL}  return keyword_else;
break{TRAIL}        return keyword_break;
emit{TRAIL}         return keyword_emit;
return{TRAIL}       return keyword_return;
nil{TRAIL}          return keyword_nil;
true{TRAIL}         return keyword_true;
false{TRAIL}        return keyword_false;

[A-Za-z_][A-Za-z0-9_]* {
  yylval->str = strdup0(yytext);
  return identifier;
};

{TRAIL}\.{TRAIL} return '.';
[(\[{,;:]{TRAIL} return yytext[0];
[)\]}]           return yytext[0];
"\n"             return '\n';

(([1-9][0-9]*)|0)(\.[0-9]*)? {
  double temp;
  sscanf(yytext, "%lf", &temp);
  yylval->d = temp;
  return lit_number;
};

\"([^\\\"]|\\.)*\"      {
  yylval->str = strndup0(yytext+1, strlen(yytext)-2);
  return lit_string;
};

[ \t] ; 

. { 
    fprintf(stderr, "lexical error ('%s').\n", yytext); 
    exit(1); 
};
%% 
