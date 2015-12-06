/*
 * Defines both the test fixture to be inherited by tests and the test runner to be called form main()
 */

#pragma once

#include <vector>


class TestFixture
{
	bool failed;
	const char* testName;

protected:
	void logTestStart();
	void logTestFail(const char* messageText, size_t line, const char* functionName, const char* fileName);
	void logTestEnd();

	bool ensure(bool condition, const char* condText, size_t line, const char* functionName, const char* fileName);
	void genericFail(const char *message, size_t line, const char* functionName, const char* fileName);

	virtual void setupFixture() {}
	virtual void test() = 0;
	virtual void tearDownFixture() {}

public:
	TestFixture();

	void runTest();
	void setName(const char* name);
	bool didFail() const { return failed; }
};


class TestRunner
{
	bool failed;
	std::vector<TestFixture*> testList;

protected:
	void addTest(TestFixture* test);
	virtual void initTests() = 0;

public:
	TestRunner();
	virtual ~TestRunner();
	
	void runTests();
	bool didFail() const { return failed; }
};


#define TESTRUNNER(NAME) class NAME : public TestRunner {\
	void initTests() override {
#define RUN_TEST(NAME) {TestFixture* t=new NAME ;t->setName(#NAME);addTest(t);}
#define END_TESTRUNNER }};


#define ENSURE(COND) {if(!ensure((COND), #COND, __LINE__, __FUNCTION__, __FILE__)) return;}
#define TEST_CHECK(FUNC_CALL) { FUNC_CALL; if(didFail()) return; }