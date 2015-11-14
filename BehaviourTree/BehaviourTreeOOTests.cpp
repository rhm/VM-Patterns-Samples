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

		BTWorldDataTest() {}
		BTWorldDataTest(std::initializer_list<Entry> _entries)
			: entries(_entries)
		{}

		static bool compare(const BTWorldDataTest& lhs, const BTWorldDataTest& rhs);
	};

	bool BTWorldDataTest::compare(const BTWorldDataTest& reference, const BTWorldDataTest& generated)
	{
		if (reference.entries.size() > generated.entries.size()) return false;

		for (size_t i = 0; i <  reference.entries.size(); ++i)
		{
			if (reference.entries[i].name != generated.entries[i].name ||
				reference.entries[i].num  != generated.entries[i].num)
			{
				return false;
			}
		}

		return true;
	}


	class BTBehaviourTestExec : public BTBehaviourExec
	{
		const char* name;
		uint32_t currCount;

	public:
		BTBehaviourTestExec(const char* _name, uint32_t initialCount)
			: name(_name)
			, currCount(initialCount)
		{}

		virtual eBTResult execute(BTBehaviourContext& context) override;
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

	eBTResult BTBehaviourTestExec::execute(BTBehaviourContext& context)
	{
		static_cast<BTWorldDataTest*>(context.worldData)->log(Name(name), currCount);

		std::cout << "Behaviour = " << name << "; Count = " << currCount << std::endl;
		--currCount;

		return (currCount > 0) ? eBTResult::InProgress : eBTResult::Success;
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
		VariablePack *vars;

		virtual void setupFixture();
		virtual void test();

		void testSequence1();
		void testSelector1();
	};
	
	void BehaviourTreeOOTest::setupFixture()
	{
		layout.addVariable(Name("branch"), eExpType::NUMBER);

		vars = new VariablePack(&layout, Name(), 0);
	}

	void BehaviourTreeOOTest::test()
	{
		SUB_TEST(testSequence1)
		SUB_TEST(testSelector1)
	}

	void BehaviourTreeOOTest::testSequence1()
	{
		BTTreeRuntimeData bt(
			vars->getLayout(), 
			new BTSequenceNode("root-seq",
				{
					new BTBehaviourNode("count1", new BTBehaviourTestSpec(1)),
					new BTBehaviourNode("count2", new BTBehaviourTestSpec(2)),
					new BTBehaviourNode("count3", new BTBehaviourTestSpec(3))
				}
			));
		
		BTWorldDataTest sampleData({
			{ Name("count1"), 1 },
			{ Name("count2"), 2 },
			{ Name("count2"), 1 },
			{ Name("count3"), 3 },
			{ Name("count3"), 2 },
			{ Name("count3"), 1 }
		});
		BTWorldDataTest generatedTestData;

		BTEvalEngine eval(&bt, &generatedTestData, vars);
		if (eval.errors().errorCount() > 0)
		{
			GENERIC_FAIL("Compile error")
		}

		for (int i = 0; i < 4; ++i)
		{
			eval.evaluate();
		}

		if (!BTWorldDataTest::compare(sampleData, generatedTestData))
		{
			GENERIC_FAIL("Incorrect output")
		}
	}

	void BehaviourTreeOOTest::testSelector1()
	{
		BTTreeRuntimeData bt(
			vars->getLayout(), 
			new BTSelectorNode("root-sel",
				{
					new BTSequenceNode("seq1", {
						new BTConditionNode("cond1", "branch == 1"),
						new BTBehaviourNode("count1", new BTBehaviourTestSpec(1)),
					}),
					new BTSequenceNode("seq2", {
						new BTConditionNode("cond2", "branch == 2"),
						new BTBehaviourNode("count2", new BTBehaviourTestSpec(2)),
					}),
					new BTSequenceNode("seq3", {
						new BTConditionNode("cond3", "branch == 3"),
						new BTBehaviourNode("count3", new BTBehaviourTestSpec(3)),
					}),
				}
			));
		
		BTWorldDataTest sampleData({
			{ Name("count2"), 2 },
			{ Name("count1"), 1 },
			{ Name("count2"), 2 },
			{ Name("count2"), 1 },
			{ Name("count2"), 2 },
		});
		BTWorldDataTest generatedTestData;

		BTEvalEngine eval(&bt, &generatedTestData, vars);
		if (eval.errors().errorCount() > 0)
		{
			GENERIC_FAIL("Compile error")
		}

		vars->setVariable(Name("branch"), 0.f);
		eval.evaluate(); // nothing should be generated
		vars->setVariable(Name("branch"), 2.f);
		eval.evaluate();
		vars->setVariable(Name("branch"), 1.f);
		eval.evaluate();
		vars->setVariable(Name("branch"), 2.f);
		eval.evaluate();
		eval.evaluate();
		eval.evaluate();

		if (!BTWorldDataTest::compare(sampleData, generatedTestData))
		{
			GENERIC_FAIL("Incorrect output")
		}	
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


