%{
 
/*
 * Lexer.l file
 * To generate the lexical analyzer run: "flex Lexer.l"
 */

#include "AST.h"
#include "GeneratedFiles/FormulaParser.h"

#include <stdio.h>

char *copyString(const char *start, size_t len); 

%}

%option outfile="GeneratedFiles/FormulaLexer.c" header-file="GeneratedFiles/FormulaLexer.h"
%option warn nodefault
 
%option reentrant noyywrap never-interactive nounistd
%option bison-bridge
 
WS          [ \r\n\t]*
 
%%
 
{WS}            { /* Skip blanks. */ }

[0-9]+ |
[0-9]+"."[0-9]* |
"."[0-9]+       { sscanf_s(yytext, "%f", &yylval->f_value); return TOKEN_NUMBER; }

\'.*\'			{ yylval->n_value = copyString(yytext+1, yyleng-2); return TOKEN_NAME; }

[a-zA-Z]+[a-zA-Z0-9_]*	{ yylval->n_value = copyString(yytext, yyleng); return TOKEN_ID; }

"("				{ return TOKEN_LPAREN; }
")"				{ return TOKEN_RPAREN; }
"+"				{ return TOKEN_PLUS; }
"-"				{ return TOKEN_MINUS; }
"*"				{ return TOKEN_MUL; }
"/"				{ return TOKEN_DIV; }
"%"				{ return TOKEN_PERCENT; }
"&&"			{ return TOKEN_AND; }
"||"			{ return TOKEN_OR; }
"!"				{ return TOKEN_NOT; }
"=="			{ return TOKEN_EQ; }
"!="			{ return TOKEN_NEQ; }
"<"				{ return TOKEN_LT; }
"<="			{ return TOKEN_LTEQ; }
">"				{ return TOKEN_GT; }
">="			{ return TOKEN_GTEQ; }

.				{ return TOKEN_ERR; }
 
%%
 
int yyerror(const char *msg) {
    fprintf(stderr,"Error:%s\n",msg); return 0;
}

char *copyString(const char *start, size_t len)
{
	char *result = (char*)malloc(len + 1);
	memcpy(result, start, len);
	result[len] = 0;
	return result;
}
