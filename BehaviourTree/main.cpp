// Formulas.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>

#include "ExpressionTests.h"
#include "BehaviourTreeTests.h"

 
int main(int argc, char* argv[])
{
	if (argc >= 2 && _stricmp(argv[1], "test") == 0)
	{
		int expressionTestResult = runExpressionTests();
		if (expressionTestResult > 0) return expressionTestResult;

		int behaviourTreeOOResults = runBehaviourTreeOOTests();
		if (behaviourTreeOOResults > 0) return behaviourTreeOOResults;

		int behaviourTreeVMResults = runBehaviourTreeVMTests();
		if (behaviourTreeVMResults > 0) return behaviourTreeVMResults;

		return 0;
	}

    return 10;
}