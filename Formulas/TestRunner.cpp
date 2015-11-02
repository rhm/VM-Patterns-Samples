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

bool TestFixture::ensure(bool condition, const char* condText, size_t line, const char* functionName, const char* fileName)
{
	if (!condition)
	{
		failed = true;

		std::cout << 
			"Error: " << condText << 
			"; file=" << fileName << 
			"; function=" << functionName <<
			"; line=" << line;
	}

	return condition;
}

void TestFixture::fail(const char* message)
{
	failed = true;
	std::cout << "Error: " << message;
}

void TestFixture::runTest()
{
	std::cout << "Beginning test: " << testName << std::endl;

	setupFixture();
	test();
	tearDownFixture();

	std::cout << std::endl;
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
