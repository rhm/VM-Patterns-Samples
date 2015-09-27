%{
 
/*
 * Parser.y file
 * To generate the parser run: "bison Parser.y"
 */
#define YY_NO_UNISTD_H

#include "Expression.h"
#include "GeneratedFiles/FormulaParser.h"
#include "GeneratedFiles/FormulaLexer.h"

int yyerror(ASTNode **expression, yyscan_t scanner, const char *msg) {
    // Add error handling routine as needed
	return 0;
}
 
%}

%code requires {

#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

}

%output  "GeneratedFiles/FormulaParser.c"
%defines "GeneratedFiles/FormulaParser.h"
 
%define api.pure
%lex-param   { yyscan_t scanner }
%parse-param { ASTNode **expression }
%parse-param { yyscan_t scanner }

%union {
    float value;
    ASTNode *expression;
}
 
%left '+' TOKEN_PLUS
%left '*' TOKEN_MULTIPLY
 
%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_PLUS
%token TOKEN_MULTIPLY
%token <value> TOKEN_NUMBER

%type <expression> expr
 
%%
 
input
    : expr { *expression = $1; }
    ;
 
expr
    : expr TOKEN_PLUS expr { $$ = createNode( eASTNodeType::ARITH_ADD, $1, $3 ); }
    | expr TOKEN_MULTIPLY expr { $$ = createNode( eASTNodeType::ARITH_MUL, $1, $3 ); }
    | TOKEN_LPAREN expr TOKEN_RPAREN { $$ = $2; }
    | TOKEN_NUMBER { $$ = new ASTNodeConst($1); }
	;
 
%%
