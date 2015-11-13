%{
 
/*
 * Parser.y file
 * To generate the parser run: "bison Parser.y"
 */
#define YY_NO_UNISTD_H

#include "AST.h"
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
    float f_value;
	char *n_value;
    ASTNode *expression;
}

%left TOKEN_OR
%left TOKEN_AND
%left TOKEN_EQ TOKEN_NEQ
%left TOKEN_LT TOKEN_LTEQ TOKEN_GT TOKEN_GTEQ
%left TOKEN_PLUS TOKEN_MINUS
%left TOKEN_MUL TOKEN_DIV TOKEN_PERCENT
%right TOKEN_NOT TOKEN_UNARY_NEG
 
%token TOKEN_ERR

%token TOKEN_LPAREN
%token TOKEN_RPAREN
%token TOKEN_TRUE
%token TOKEN_FALSE
%token <n_value> TOKEN_NAME
%token <f_value> TOKEN_NUMBER
%token <n_value> TOKEN_ID

%type <expression> expr
 
%%
 
input
    : expr { *expression = $1; }
    ;
 
expr
    : expr TOKEN_PLUS expr		{ $$ = createNode( eASTNodeType::ARITH_ADD, $1, $3 ); }
    | expr TOKEN_MINUS expr		{ $$ = createNode( eASTNodeType::ARITH_SUB, $1, $3 ); }
	| expr TOKEN_MUL expr		{ $$ = createNode( eASTNodeType::ARITH_MUL, $1, $3 ); }
	| expr TOKEN_DIV expr		{ $$ = createNode( eASTNodeType::ARITH_DIV, $1, $3 ); }
	| expr TOKEN_PERCENT expr	{ $$ = createNode( eASTNodeType::ARITH_MOD, $1, $3 ); }
	| expr TOKEN_AND expr		{ $$ = createNode( eASTNodeType::LOGICAL_AND, $1, $3 ); }
	| expr TOKEN_OR expr		{ $$ = createNode( eASTNodeType::LOGICAL_OR, $1, $3 ); }
	| TOKEN_NOT expr			{ $$ = createNode( eASTNodeType::LOGICAL_NOT, $2, nullptr ); }
	| TOKEN_MINUS expr %prec TOKEN_UNARY_NEG { $$ = createNode( eASTNodeType::ARITH_SUB, createConstNode(0.f), $2 ); }
	| expr TOKEN_EQ expr		{ $$ = createNode( eASTNodeType::COMP_EQ, $1, $3 ); }
	| expr TOKEN_NEQ expr		{ $$ = createNode( eASTNodeType::COMP_NEQ, $1, $3 ); }
	| expr TOKEN_LT expr		{ $$ = createNode( eASTNodeType::COMP_LT, $1, $3 ); }
	| expr TOKEN_LTEQ expr		{ $$ = createNode( eASTNodeType::COMP_LTEQ, $1, $3 ); }
	| expr TOKEN_GT expr		{ $$ = createNode( eASTNodeType::COMP_GT, $1, $3 ); }
	| expr TOKEN_GTEQ expr		{ $$ = createNode( eASTNodeType::COMP_GTEQ, $1, $3 ); }
    | TOKEN_LPAREN expr TOKEN_RPAREN { $$ = $2; }
    | TOKEN_NUMBER				{ $$ = createConstNode($1); }
	| TOKEN_NAME				{ $$ = createConstNode($1); free((void*)$1); }
	| TOKEN_ID					{ $$ = createIDNode($1); free((void*)$1); }
	| TOKEN_TRUE				{ $$ = createConstNode(true); }
	| TOKEN_FALSE				{ $$ = createConstNode(false); }
	;
 
%%
