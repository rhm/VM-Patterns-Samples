// Formulas.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "Expression.h"
#include "GeneratedFiles/FormulaParser.h"
#include "GeneratedFiles/FormulaLexer.h"
 
#include <stdio.h>
 
int yyparse(ASTNode **expression, yyscan_t scanner);
 
ASTNode *getAST(const char *expr)
{
    ASTNode *expression;
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) {
        // couldn't initialize
        return NULL;
    }
 
    state = yy_scan_string(expr, scanner);
 
    if (yyparse(&expression, scanner)) {
        // error parsing
        return NULL;
    }
 
    yy_delete_buffer(state, scanner);
 
    yylex_destroy(scanner);
 
    return expression;
}
 
 
int _tmain(int argc, _TCHAR* argv[])
{
    ASTNode *e = NULL;
    const char test[]=" 4 + 2*10 + 3*( 5 + 1 )";
    int result = 0;
 
    e = getAST(test);
 
    //result = evaluate(e);
 
    //printf("Result of '%s' is %d\n", test, result);
 
    delete e;
 
    return 0;
}