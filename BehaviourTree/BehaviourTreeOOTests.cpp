/*
 * Unit tests for the Expresion system
 */

#include "stdafx.h"

#include <iostream>
#include <memory>

#include "BehaviourTreeOOTests.h"
#include "TestRunner.h"

#include "BehaviourTreeOO.h"


namespace BehaviourTreeOO
{
	class BTWorldDataTest : public BTWorldData
	{
	public:
		struct Entry
		{
			Name name;
			uint32_t num;

			Entry(Name _name, uint32_t _num)
				: name(_name)
				, num(_num)
			{}
		};

		std::vector<Entry> entries;

		void log(Name name, uint32_t num)
		{
			entries.emplace_back(name, num);
		}
	};


	class BTBehaviourTestExec : public BTBehaviourExec
	{
		const char* name;
		uint32_t currCount;

	public:
		BTBehaviourTestExec(const char* _name, uint32_t initialCount)
			: name(_name)
			, currCount(initialCount)
		{}

		virtual void init(const BTBehaviourNode* originNode, BTBehaviourContext& context) override;
		virtual eBTResult execute(BTBehaviourContext& context) override;
		virtual void cleanUp(BTBehaviourContext& context) override;
	};

	class BTBehaviourTestSpec : public BTBehaviourSpec
	{
		uint32_t initialCount;

	public:
		BTBehaviourTestSpec(uint32_t _initialCount)
			: initialCount(_initialCount)
		{}

		virtual BTBehaviourExec* getNewExec(const BTBehaviourNode* originNode, BTBehaviourContext& context) const override;
	};

	void BTBehaviourTestExec::init(const BTBehaviourNode* originNode, BTBehaviourContext& context)
	{
		// do nothing
	}

	eBTResult BTBehaviourTestExec::execute(BTBehaviourContext& context)
	{
		static_cast<BTWorldDataTest*>(context.worldData)->log(Name(name), currCount);

		std::cout << "Behaviour=" << name << "; Count" << currCount;
		--currCount;

		return (currCount > 0) ? eBTResult::InProgress : eBTResult::Success;
	}

	void BTBehaviourTestExec::cleanUp(BTBehaviourContext& context)
	{
		// do nothing
	}
	

	BTBehaviourExec* BTBehaviourTestSpec::getNewExec(const BTBehaviourNode* originNode, BTBehaviourContext& context) const
	{
		return new BTBehaviourTestExec(originNode->getNodeName(), initialCount);
	}


	/*
	 * BehaviourTreeOOTests
	 */

	class BehaviourTreeOOTest : public TestFixture
	{
	protected:
		VariableLayout layout;

		virtual void setupFixture();
		virtual void test();
	};


	void BehaviourTreeOOTest::setupFixture()
	{
		layout.addVariable(Name("NumA"), eExpType::NUMBER);
		layout.addVariable(Name("NumB"), eExpType::NUMBER);
		layout.addVariable(Name("NumC"), eExpType::NUMBER);

		layout.addVariable(Name("NameC"), eExpType::NAME);
		layout.addVariable(Name("NameC2"), eExpType::NAME);
		layout.addVariable(Name("NameD"), eExpType::NAME);
	}

	void BehaviourTreeOOTest::test()
	{



	}






	/*
	 * TestRunner
	 */

	TESTRUNNER(BehaviourTreeOOTests)
		RUN_TEST(BehaviourTreeOOTest)
	END_TESTRUNNER

} // namespace BehaviourTreeOO


int runBehaviourTreeOOTests()
{
	BehaviourTreeOO::BehaviourTreeOOTests tr;

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


