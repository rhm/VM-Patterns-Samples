/*
 * Unit tests for the Expresion system
 */

#include "stdafx.h"

#include <sstream>
#include <memory>

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
	void trialCompileExpectFail(const char* expressionText, size_t line, const char* functionName, const char* fileName, eErrorCode expectedErrorCode);
	
	virtual void setupFixture();
};

ExpressionData* ExpressionTestBase::compile(const char* expressionText, size_t line, const char* functionName, const char* fileName)
{
	ExpressionCompiler comp(&layout);
	std::unique_ptr<ExpressionData> expData(comp.compile(expressionText));

	if (comp.errors().errorCount() > 0) 
	{
		std::ostringstream msg;
		msg << "Compile error - " << comp.errors().error(0).message;
		genericFail(msg.str().c_str(), line, functionName, fileName);
	}

	return expData.release();
}

void ExpressionTestBase::trialCompile(const char* expressionText, size_t line, const char* functionName, const char* fileName)
{
	std::unique_ptr<ExpressionData> expData(compile(expressionText, line, functionName, fileName));
}

void ExpressionTestBase::trialCompileExpectFail(const char* expressionText, size_t line, const char* functionName, const char* fileName, eErrorCode expectedErrorCode)
{
	ExpressionCompiler comp(&layout);
	std::unique_ptr<ExpressionData> expData(comp.compile(expressionText));
	
	if (comp.errors().errorCount() == 0) 
	{
		genericFail("Compile expected error, none reported", line, functionName, fileName);
	}
	else if (comp.errors().error(0).code != expectedErrorCode)
	{
		genericFail("Compile expected one error but got another", line, functionName, fileName);
	}
}


void ExpressionTestBase::setupFixture()
{
	layout.addVariable(Name("NumA"), eExpType::NUMBER);
	layout.addVariable(Name("NumB"), eExpType::NUMBER);
	layout.addVariable(Name("NumC"), eExpType::NUMBER);

	layout.addVariable(Name("NameC"), eExpType::NAME);
	layout.addVariable(Name("NameC2"), eExpType::NAME);
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
	void executeExpectError(const char* expressionText, size_t line, const char* functionName, const char* fileName, eErrorCode expectedErrorCode);

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
	vars->setVariable(Name("NumC"), 2.f);

	vars->setVariable(Name("NameC"), Name("C"));
	vars->setVariable(Name("NameC2"), Name("C"));
	vars->setVariable(Name("NameD"), Name("D"));
}

void ExecutionTests::tearDownFixture()
{
	delete vars;
}

void ExecutionTests::executeNumber(const char* expressionText, size_t line, const char* functionName, const char* fileName, float expectedValue)
{
	std::unique_ptr<ExpressionData> expData(compile(expressionText, line, functionName, fileName));
	if (didFail()) return;

	ExpressionEvaluator eval(vars);
	eval.evaluate(expData.get());

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
	std::unique_ptr<ExpressionData> expData(compile(expressionText, line, functionName, fileName));
	if (didFail()) return;

	ExpressionEvaluator eval(vars);
	eval.evaluate(expData.get());

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

void ExecutionTests::executeExpectError(const char* expressionText, size_t line, const char* functionName, const char* fileName, eErrorCode expectedErrorCode)
{
	ExpressionCompiler comp(&layout);
	std::unique_ptr<ExpressionData> expData(comp.compile(expressionText));
		
	if (comp.errors().errorCount() > 0) 
	{
		if (comp.errors().error(0).code == expectedErrorCode)
		{
			return;
		}
		else
		{
			genericFail("Compile expected one error but got another", line, functionName, fileName);
			return;
		}
	}

	ExpressionEvaluator eval(vars);
	eval.evaluate(expData.get());

	if (eval.errors().errorCount() > 0)
	{
		if (eval.errors().error(0).code != expectedErrorCode)
		{
			genericFail("Compile expected one error but got another", line, functionName, fileName);
		}
	}
	else
	{
		genericFail("Compile expected one error but got none", line, functionName, fileName);	
	}
}


#define TEST_EXPRESSION_NUM(EXP,VALUE) { executeNumber(EXP, __LINE__, __FUNCTION__, __FILE__, VALUE); if (didFail()) return; }
#define TEST_EXPRESSION_BOOL(EXP,VALUE) { executeBool(EXP, __LINE__, __FUNCTION__, __FILE__, VALUE); if (didFail()) return; }
#define TEST_EXPRESSION_FAILS(EXP,ERRORCODE) { executeExpectError(EXP, __LINE__, __FUNCTION__, __FILE__, ERRORCODE); if (didFail()) return; }

void ExecutionTests::test()
{
	// Arithmetic

	TEST_EXPRESSION_NUM("2+4.5", 6.5);
	TEST_EXPRESSION_NUM("2+NumA", 7);
	TEST_EXPRESSION_NUM("NumA+4.5", 9.5);
	TEST_EXPRESSION_NUM("NumA+NumB", 2);

	TEST_EXPRESSION_NUM("10-7.5-1", 1.5);
	TEST_EXPRESSION_NUM("-4-5", -9);
	TEST_EXPRESSION_NUM("4+-3", 1);
	TEST_EXPRESSION_NUM("4--3", 7);

	TEST_EXPRESSION_NUM("4-NumA", -1);
	TEST_EXPRESSION_NUM("4-NumB", 7);
	TEST_EXPRESSION_NUM("NumA-NumB", 8);
	TEST_EXPRESSION_NUM("NumA--3", 8);
	
	TEST_EXPRESSION_NUM("4*2", 8);
	TEST_EXPRESSION_NUM("4*NumA", 20);
	TEST_EXPRESSION_NUM("NumA*2", 10);
	TEST_EXPRESSION_NUM("NumA*NumB", -15);

	TEST_EXPRESSION_NUM("4*3*2", 24);
	TEST_EXPRESSION_NUM("-3*2", -6);
	TEST_EXPRESSION_NUM("5*-2", -10);

	TEST_EXPRESSION_NUM("NumA/2", 2.5);
	TEST_EXPRESSION_NUM("10/NumA", 2);
	TEST_EXPRESSION_NUM("NumA/NumC", 2.5);
	TEST_EXPRESSION_NUM("10/-2", -5);
	TEST_EXPRESSION_NUM("-10/2", -5);
	TEST_EXPRESSION_NUM("-10/-2", 5);

	TEST_EXPRESSION_NUM("12 % 5", 2);
	TEST_EXPRESSION_NUM("NumA % 2", 1);
	TEST_EXPRESSION_NUM("12 % NumA", 2);
	TEST_EXPRESSION_NUM("NumA % NumC", 1);
	TEST_EXPRESSION_NUM("-12 %5", -2);
	TEST_EXPRESSION_NUM("12 % -5", 2);
	TEST_EXPRESSION_NUM("-12%-5", -2);


	// Numeric comparison

	TEST_EXPRESSION_BOOL("NumA == 5", true);
	TEST_EXPRESSION_BOOL("5==NumA", true);
	TEST_EXPRESSION_BOOL("5==5", true);
	TEST_EXPRESSION_BOOL("5 == 10/2", true);
	TEST_EXPRESSION_BOOL("10/2 ==5", true);
	TEST_EXPRESSION_BOOL("NumA == 10/2", true);
	TEST_EXPRESSION_BOOL("10/2 == NumA", true);

	TEST_EXPRESSION_BOOL("NumB == 5", false);
	TEST_EXPRESSION_BOOL("5==NumB", false);
	TEST_EXPRESSION_BOOL("5==88", false);
	TEST_EXPRESSION_BOOL("88 == 10/2", false);
	TEST_EXPRESSION_BOOL("10/2 ==88", false);
	TEST_EXPRESSION_BOOL("NumB == 10/2", false);
	TEST_EXPRESSION_BOOL("10/2 == NumB", false);

	TEST_EXPRESSION_BOOL("NumB != 5", true);
	TEST_EXPRESSION_BOOL("5!=NumB", true);
	TEST_EXPRESSION_BOOL("5!=88", true);
	TEST_EXPRESSION_BOOL("88 != 10/NumC", true);
	TEST_EXPRESSION_BOOL("10/NumC !=88", true);
	TEST_EXPRESSION_BOOL("NumB != 10/NumC", true);
	TEST_EXPRESSION_BOOL("10/NumC != NumB", true);

	TEST_EXPRESSION_BOOL("NumA != 5", false);
	TEST_EXPRESSION_BOOL("5!=NumA", false);
	TEST_EXPRESSION_BOOL("5!=5", false);
	TEST_EXPRESSION_BOOL("5 != 10/NumC", false);
	TEST_EXPRESSION_BOOL("10/NumC !=5", false);
	TEST_EXPRESSION_BOOL("NumA != 10/NumC", false);
	TEST_EXPRESSION_BOOL("10/NumC != NumA", false);

	TEST_EXPRESSION_BOOL("NumA < 7", true);
	TEST_EXPRESSION_BOOL("3 < NumA", true);
	TEST_EXPRESSION_BOOL("3 < 5", true);
	TEST_EXPRESSION_BOOL("3 < NumC*3", true);
	TEST_EXPRESSION_BOOL("20/NumA < 5", true);
	TEST_EXPRESSION_BOOL("20/NumA < NumA", true);
	TEST_EXPRESSION_BOOL("NumA < NumC*3", true);

	TEST_EXPRESSION_BOOL("NumA < 3", false);
	TEST_EXPRESSION_BOOL("5 < NumA", false);
	TEST_EXPRESSION_BOOL("5 < 5", false);
	TEST_EXPRESSION_BOOL("10 < NumC*3", false);
	TEST_EXPRESSION_BOOL("20/NumC < 1", false);
	TEST_EXPRESSION_BOOL("20/NumC < NumA", false);
	TEST_EXPRESSION_BOOL("NumA < 1+NumC", false);

	TEST_EXPRESSION_BOOL("NumA <= 7", true);
	TEST_EXPRESSION_BOOL("NumA <= 5", true);
	TEST_EXPRESSION_BOOL("3 <= NumA", true);
	TEST_EXPRESSION_BOOL("5 <= NumA", true);
	TEST_EXPRESSION_BOOL("3 <= 5", true);
	TEST_EXPRESSION_BOOL("5 <= 5", true);
	TEST_EXPRESSION_BOOL("3 <= NumC*3", true);
	TEST_EXPRESSION_BOOL("6 <= NumC*3", true);
	TEST_EXPRESSION_BOOL("10/NumC <= 10", true);
	TEST_EXPRESSION_BOOL("10/NumC <= 5", true);
	TEST_EXPRESSION_BOOL("20/NumA <= NumA", true);
	TEST_EXPRESSION_BOOL("10/NumC <= NumA", true);
	TEST_EXPRESSION_BOOL("NumA <= NumC*3", true);
	TEST_EXPRESSION_BOOL("NumA <= NumC+3", true);

	TEST_EXPRESSION_BOOL("NumA <= 3", false);
	TEST_EXPRESSION_BOOL("6 <= NumA", false);
	TEST_EXPRESSION_BOOL("10 <= 5", false);
	TEST_EXPRESSION_BOOL("10 <= NumC*3", false);
	TEST_EXPRESSION_BOOL("10/NumC <= 1", false);
	TEST_EXPRESSION_BOOL("100/NumC <= NumA", false);
	TEST_EXPRESSION_BOOL("NumA <= 1+NumC", false);
	
	TEST_EXPRESSION_BOOL("NumA > 3", true);
	TEST_EXPRESSION_BOOL("10 > NumA", true);
	TEST_EXPRESSION_BOOL("10 > 5", true);
	TEST_EXPRESSION_BOOL("10 > NumC*3", true);
	TEST_EXPRESSION_BOOL("10/NumC > 1", true);
	TEST_EXPRESSION_BOOL("100/NumC > NumA", true);
	TEST_EXPRESSION_BOOL("NumA > 1+NumC", true);

	TEST_EXPRESSION_BOOL("NumA > 7", false);
	TEST_EXPRESSION_BOOL("3 > NumA", false);
	TEST_EXPRESSION_BOOL("3 > 5", false);
	TEST_EXPRESSION_BOOL("3 > NumC*3", false);
	TEST_EXPRESSION_BOOL("10/NumC > 5", false);
	TEST_EXPRESSION_BOOL("10/NumC > NumA", false);
	TEST_EXPRESSION_BOOL("NumA > NumC*3", false);

	TEST_EXPRESSION_BOOL("NumA >= 3", true);
	TEST_EXPRESSION_BOOL("NumA >= 5", true);
	TEST_EXPRESSION_BOOL("10 >= NumA", true);
	TEST_EXPRESSION_BOOL("5 >= NumA", true);
	TEST_EXPRESSION_BOOL("10 >= 5", true);
	TEST_EXPRESSION_BOOL("5 >= 5", true);
	TEST_EXPRESSION_BOOL("10 >= NumC*3", true);
	TEST_EXPRESSION_BOOL("6 >= NumC*3", true);
	TEST_EXPRESSION_BOOL("10/NumC >= 1", true);
	TEST_EXPRESSION_BOOL("10/NumC >= 5", true);
	TEST_EXPRESSION_BOOL("20/NumC >= NumA", true);
	TEST_EXPRESSION_BOOL("10/NumC >= NumA", true);
	TEST_EXPRESSION_BOOL("NumA >= 1+NumC", true);
	TEST_EXPRESSION_BOOL("NumA >= NumC+3", true);

	TEST_EXPRESSION_BOOL("NumA >= 7", false);
	TEST_EXPRESSION_BOOL("3 >= NumA", false);
	TEST_EXPRESSION_BOOL("3 >= 5", false);
	TEST_EXPRESSION_BOOL("3 >= NumC*3", false);
	TEST_EXPRESSION_BOOL("NumA >= NumC*3", false);

	// Name equality

	TEST_EXPRESSION_BOOL("NameC == 'C'", true);
	TEST_EXPRESSION_BOOL("NameC == 'A'", false);
	TEST_EXPRESSION_BOOL("NameC != 'C'", false);

	TEST_EXPRESSION_BOOL("'C' == NameC", true);
	TEST_EXPRESSION_BOOL("'A' == NameC", false);
	TEST_EXPRESSION_BOOL("'C' != NameC", false);

	TEST_EXPRESSION_BOOL("NameC == NameC2", true);
	TEST_EXPRESSION_BOOL("NameC == NameD", false);
	TEST_EXPRESSION_BOOL("NameC != NameC2", false);

	// Bool equality

	TEST_EXPRESSION_BOOL("(NumA == 5) == (NumB < 0)", true);
	TEST_EXPRESSION_BOOL("(NumA == 5) != (NumB < 0)", false);
	TEST_EXPRESSION_BOOL("(NumA == 5) == (NumB > 0)", false);
	TEST_EXPRESSION_BOOL("(NumA == 5) != (NumB > 0)", true);
	
	
	// Logical operators
	
	TEST_EXPRESSION_BOOL("1<2 && 3>2", true);
	TEST_EXPRESSION_BOOL("1>2 && 3>2", false);
	TEST_EXPRESSION_BOOL("1<2 && 3<2", false);
	TEST_EXPRESSION_BOOL("1>2 && 3<2", false);

	TEST_EXPRESSION_BOOL("NumA==5 && 3>2", true);
	TEST_EXPRESSION_BOOL("NumA==5 && 3<2", false);
	TEST_EXPRESSION_BOOL("NumA==6 && 3>2", false);
	TEST_EXPRESSION_BOOL("NumA==6 && 3<2", false);

	TEST_EXPRESSION_BOOL("NumA==5 && NumB<0", true);
	TEST_EXPRESSION_BOOL("NumA!=5 && NumB<0", false);
	TEST_EXPRESSION_BOOL("NumA==5 && NumB>0", false);
	
	TEST_EXPRESSION_BOOL("1<2 || 3>2", true);
	TEST_EXPRESSION_BOOL("1<2 || 3<2", true);
	TEST_EXPRESSION_BOOL("1>2 || 3>2", true);
	TEST_EXPRESSION_BOOL("1>2 || 3<2", false);

	TEST_EXPRESSION_BOOL("NumA==5 || 3>2", true);
	TEST_EXPRESSION_BOOL("NumA==5 || 3<2", true);
	TEST_EXPRESSION_BOOL("NumA!=5 || 3>2", true);
	TEST_EXPRESSION_BOOL("NumA!=5 || 3<2", false);

	TEST_EXPRESSION_BOOL("3>2 || NumA==5", true);
	TEST_EXPRESSION_BOOL("3<2 || NumA==5", true);
	TEST_EXPRESSION_BOOL("3>2 || NumA!=5", true);
	TEST_EXPRESSION_BOOL("3<2 || NumA!=5", false);

	TEST_EXPRESSION_BOOL("NumA==5 || NumB<0", true);
	TEST_EXPRESSION_BOOL("NumA==5 || NumB>0", true);
	TEST_EXPRESSION_BOOL("NumA!=5 || NumB<0", true);
	TEST_EXPRESSION_BOOL("NumA!=5 || NumB>0", false);


	// Tests error reporting

	TEST_EXPRESSION_FAILS("'A'", eErrorCode::ConstNameExpression);
	TEST_EXPRESSION_FAILS("NameC", eErrorCode::ConstNameExpression);

	TEST_EXPRESSION_FAILS("5/0", eErrorCode::DivideByZero);
	TEST_EXPRESSION_FAILS("NumA/(NumA-5)", eErrorCode::DivideByZero);
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