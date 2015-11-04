/*
 * Unit tests for the Expresion system
 */

#include "stdafx.h"

#include <sstream>

#include "ExpressionTests.h"
#include "TestRunner.h"

#include "Expression.h"


/*
 * ParsingTests
 */

class CompileTests : public TestFixture
{
	VariableLayout layout;

	void compile(const char* expressionText, size_t line, const char* functionName, const char* fileName);

protected:
	virtual void setupFixture();
	virtual void test();
};

void CompileTests::compile(const char* expressionText, size_t line, const char* functionName, const char* fileName)
{
	ExpressionCompiler comp(&layout);
	ExpressionData *expData = comp.compile(expressionText);
	delete expData;

	if (comp.errors().errorCount() > 0) 
	{
		std::ostringstream msg;
		msg << "Compile error - " << comp.errors().error(0).message;
		genericFail(msg.str().c_str(), line, functionName, fileName);
	}
}

void CompileTests::setupFixture()
{
	layout.addVariable(Name("NumA"), eExpType::NUMBER);
	layout.addVariable(Name("NumB"), eExpType::NUMBER);

	layout.addVariable(Name("NameC"), eExpType::NAME);
	layout.addVariable(Name("NameD"), eExpType::NAME);
}

#define TEST_COMPILE(EXP) { compile(EXP, __LINE__, __FUNCTION__, __FILE__); if (didFail()) return; }

void CompileTests::test()
{
	TEST_COMPILE("4+NumA");
	TEST_COMPILE("-3.4 - 5");
	TEST_COMPILE("-3.4-5");
	TEST_COMPILE("-3+-3.6444");
	TEST_COMPILE("NumA*NumB");
	TEST_COMPILE("NumA*(NumB/2.3)");
	TEST_COMPILE("NumA % 3 == 1");
	TEST_COMPILE("NumA == NumB");
	TEST_COMPILE("3 != NumB -1");
	TEST_COMPILE("4 < 5");
	TEST_COMPILE("4 <= 65");
	TEST_COMPILE("4 > 56");
	TEST_COMPILE("4 >=45");
	TEST_COMPILE("4 == NumA && NumA<=NumB");
	TEST_COMPILE("4 == NumA && NumA<=NumB/2");
	TEST_COMPILE("NumA > 3 || NumB > 3 && NumA<0");
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