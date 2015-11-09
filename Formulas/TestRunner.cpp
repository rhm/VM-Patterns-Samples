/* 
 * Unit test framework
 *
 */

#include "stdafx.h"

#include <iostream>
#include "TestRunner.h"


/*
 * TestFixture
 *
 */

TestFixture::TestFixture()
	: failed(false)
{}

void TestFixture::logTestStart()
{
	std::cout << "Beginning test: " << testName << std::endl;
}

void TestFixture::logTestFail(const char* messageText, size_t line, const char* functionName, const char* fileName)
{
	std::cout << 
		"Error: " << messageText << std::endl <<
		"    file=" << fileName << std::endl <<
		"    function=" << functionName << std::endl <<
		"    line=" << line << std::endl <<
		std::endl;
}

void TestFixture::logTestEnd()
{
	std::cout << "End test: " << (didFail() ? "FAILED" : "PASSED") << std::endl;
}

bool TestFixture::ensure(bool condition, const char* condText, size_t line, const char* functionName, const char* fileName)
{
	if (!condition)
	{
		failed = true;
		logTestFail(condText, line, functionName, fileName);
	}

	return condition;
}

void TestFixture::genericFail(const char *message, size_t line, const char* functionName, const char* fileName)
{
	failed = true;
	logTestFail(message, line, functionName, fileName);
}

void TestFixture::runTest()
{
	logTestStart();

	setupFixture();
	test();
	tearDownFixture();

	logTestEnd();
}

void TestFixture::setName(const char* name)
{
	testName = name;
}



/*
 * TestRunner
 *
 */

TestRunner::TestRunner()
	: failed(false)
{}

TestRunner::~TestRunner()
{
	for (TestFixture* t : testList)
	{
		delete t;
	}
}

void TestRunner::addTest(TestFixture* test)
{
	testList.push_back(test);
}

void TestRunner::runTests()
{
	initTests();

	for (TestFixture* t : testList)
	{
		t->runTest();
		if (t->didFail())
		{
			failed = true;
			return;
		}
	}
}
