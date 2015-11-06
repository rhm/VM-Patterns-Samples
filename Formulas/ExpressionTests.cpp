/*
 * Unit tests for the Expresion system
 */

#include "stdafx.h"

#include <sstream>

#include "ExpressionTests.h"
#include "TestRunner.h"

#include "Expression.h"


/*
 * ExpressionTestBase
 */

class ExpressionTestBase : public TestFixture
{
protected:
	VariableLayout layout;

	ExpressionData* compile(const char* expressionText, size_t line, const char* functionName, const char* fileName);
	void trialCompile(const char* expressionText, size_t line, const char* functionName, const char* fileName);
	
	virtual void setupFixture();
};

ExpressionData* ExpressionTestBase::compile(const char* expressionText, size_t line, const char* functionName, const char* fileName)
{
	ExpressionCompiler comp(&layout);
	ExpressionData *expData = comp.compile(expressionText);

	if (comp.errors().errorCount() > 0) 
	{
		std::ostringstream msg;
		msg << "Compile error - " << comp.errors().error(0).message;
		genericFail(msg.str().c_str(), line, functionName, fileName);
	}

	return expData;
}

void ExpressionTestBase::trialCompile(const char* expressionText, size_t line, const char* functionName, const char* fileName)
{
	ExpressionData* expData = compile(expressionText, line, functionName, fileName);
	if (expData != nullptr)
	{
		delete expData;
	}
}

void ExpressionTestBase::setupFixture()
{
	layout.addVariable(Name("NumA"), eExpType::NUMBER);
	layout.addVariable(Name("NumB"), eExpType::NUMBER);

	layout.addVariable(Name("NameC"), eExpType::NAME);
	layout.addVariable(Name("NameD"), eExpType::NAME);
}


/*
 * Compilation Tests
 */

class CompileTests : public ExpressionTestBase
{
protected:
	virtual void test();
};

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
 * Execution Tests
 */

class ExecutionTests : public ExpressionTestBase
{
	VariablePack *vars;

protected:
	void executeNumber(const char* expressionText, size_t line, const char* functionName, const char* fileName, float expectedValue);
	void executeBool(const char* expressionText, size_t line, const char* functionName, const char* fileName, bool expectedValue);

	virtual void setupFixture();
	virtual void test();
	virtual void tearDownFixture();
};

void ExecutionTests::setupFixture()
{
	ExpressionTestBase::setupFixture();

	vars = new VariablePack(&layout, Name(), 0);

	vars->setVariable(Name("NumA"), 5.f);
	vars->setVariable(Name("NumB"), -3.f);

	vars->setVariable(Name("NameC"), Name("C"));
	vars->setVariable(Name("NameD"), Name("D"));
}

void ExecutionTests::tearDownFixture()
{
	delete vars;
}

void ExecutionTests::executeNumber(const char* expressionText, size_t line, const char* functionName, const char* fileName, float expectedValue)
{
	ExpressionData* expData = compile(expressionText, line, functionName, fileName);
	if (didFail()) return;

	ExpressionEvaluator eval(vars);
	eval.evaluate(expData);
	delete expData;

	if (eval.errors().errorCount() > 0)
	{
		std::ostringstream msg;
		msg << "Expression error - " << eval.errors().error(0).message;
		genericFail(msg.str().c_str(), line, functionName, fileName);
		return;
	}

	if (eval.getResultType() != eExpType::NUMBER)
	{
		genericFail("Expression result type not numeric", line, functionName, fileName);
		return;
	}

	if (eval.getNumericResult() != expectedValue)
	{
		std::ostringstream msg;
		msg << "Expected result: " << expectedValue << ", actual: " << eval.getNumericResult();
		genericFail(msg.str().c_str(), line, functionName, fileName);
		return;
	}
}

void ExecutionTests::executeBool(const char* expressionText, size_t line, const char* functionName, const char* fileName, bool expectedValue)
{
	ExpressionData* expData = compile(expressionText, line, functionName, fileName);
	if (didFail()) return;

	ExpressionEvaluator eval(vars);
	eval.evaluate(expData);
	delete expData;

	if (eval.errors().errorCount() > 0)
	{
		std::ostringstream msg;
		msg << "Expression error - " << eval.errors().error(0).message;
		genericFail(msg.str().c_str(), line, functionName, fileName);
		return;
	}

	if (eval.getResultType() != eExpType::BOOL)
	{
		genericFail("Expression result type not numeric", line, functionName, fileName);
		return;
	}

	if (eval.getBoolResult() != expectedValue)
	{
		std::ostringstream msg;
		msg << "Expected result: " << expectedValue << ", actual: " << eval.getBoolResult();
		genericFail(msg.str().c_str(), line, functionName, fileName);
		return;
	}
}


#define TEST_EXPRESSION_NUM(EXP,VALUE) { executeNumber(EXP, __LINE__, __FUNCTION__, __FILE__, VALUE); if (didFail()) return; }
#define TEST_EXPRESSION_BOOL(EXP,VALUE) { executeBool(EXP, __LINE__, __FUNCTION__, __FILE__, VALUE); if (didFail()) return; }

void ExecutionTests::test()
{
	//TEST_EXPRESSION_NUM("4+7", 11);

	TEST_EXPRESSION_BOOL("NumA == 5", true);
	TEST_EXPRESSION_BOOL("NumA < 3.5", false);
}




/*
 * TestRunner
 */

TESTRUNNER(ExpressionTests)
	RUN_TEST(CompileTests)
	RUN_TEST(ExecutionTests)
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