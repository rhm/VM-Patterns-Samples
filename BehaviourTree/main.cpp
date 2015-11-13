// Formulas.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <stdio.h>
#include <string.h>

#include "ExpressionTests.h"

 
int main(int argc, char* argv[])
{
	if (argc >= 2 && _stricmp(argv[1], "test") == 0)
	{
		return runExpressionTests();
	}

    return 10;
}