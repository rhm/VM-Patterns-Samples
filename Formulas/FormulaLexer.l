%{
 
/*
 * Lexer.l file
 * To generate the lexical analyzer run: "flex Lexer.l"
 */

#include "Expression.h"
#include "GeneratedFiles/FormulaParser.h"

#include <stdio.h>
 
%}

%option outfile="GeneratedFiles/FormulaLexer.c" header-file="GeneratedFiles/FormulaLexer.h"
%option warn nodefault
 
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge
 
LPAREN      "("
RPAREN      ")"
PLUS        "+"
MULTIPLY    "*"

WS          [ \r\n\t]*
 
%%
 
{WS}            { /* Skip blanks. */ }

-?[0-9]+ |
-?[0-9]+"."[0-9]* |
-?"."[0-9]+     { sscanf_s(yytext, "%f", &yylval->value); return TOKEN_NUMBER; }

{MULTIPLY}      { return TOKEN_MULTIPLY; }
{PLUS}          { return TOKEN_PLUS; }
{LPAREN}        { return TOKEN_LPAREN; }
{RPAREN}        { return TOKEN_RPAREN; }
.               {  }
 
%%
 
int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}
