/*
 * Unit tests for the Expresion system
 */

#include "stdafx.h"

#include "ExpressionTests.h"
#include "TestRunner.h"

#include "Expression.h"


/*
 * ParsingTests
 */

class CompileTests : public TestFixture
{
	VariableLayout layout;

	void compile(const char* expressionText);

protected:
	virtual void setupFixture();
	virtual void test();
};

void CompileTests::compile(const char* expressionText)
{
	ExpressionCompiler comp(&layout);
	comp.compile(expressionText);

	if (comp.errors().errorCount() > 0) 
		fail("Parse error");
}

void CompileTests::setupFixture()
{
	layout.addVariable(Name("NumA"), eExpType::NUMBER);
	layout.addVariable(Name("NumB"), eExpType::NUMBER);

	layout.addVariable(Name("NameC"), eExpType::NAME);
	layout.addVariable(Name("NameD"), eExpType::NAME);
}

void CompileTests::test()
{
	TEST_CHECK(compile("4+NumA"));
}



/*
 * TestRunner
 */

TESTRUNNER(ExpressionTests)
	RUN_TEST(CompileTests)
END_TESTRUNNER


int runExpressionTests()
{
	ExpressionTests tr;

	tr.runTests();

	if (tr.didFail())
	{
		return -1;
	}
	else
	{
		return 0;
	}
}