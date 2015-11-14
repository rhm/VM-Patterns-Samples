// Formulas.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>

#include "ExpressionTests.h"
#include "BehaviourTreeOOTests.h"

 
int main(int argc, char* argv[])
{
	if (argc >= 2 && _stricmp(argv[1], "test") == 0)
	{
		int expressionTestResult = runExpressionTests();
		if (expressionTestResult > 0) return expressionTestResult;

		int behaviourTreeOOResults = runBehaviourTreeOOTests();
		if (behaviourTreeOOResults > 0) return behaviourTreeOOResults;

		return 0;
	}

    return 10;
}