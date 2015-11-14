/*
 * Behaviour tree system implemented in an OO style, except for the formula evaluation, which uses the Expression VM.
 */

#include "stdafx.h"

#include <unordered_map>
#include <iostream>

#include "BehaviourTreeOO.h"



namespace BehaviourTreeOO
{

	/*
	 * BTEvalContext
	 */

	struct BTEvalContext : public BTBehaviourContext
	{
		eBTResult lastResult;
		BTBehaviourSpec* currBehaviourSpec;
		BTBehaviourExec* currBehaviourExec;

		std::unordered_map<const BTNode*, size_t> seqIndex;

		BTEvalContext(BTErrorReporter* _errorReporter,  BTWorldData* _worldData, const VariablePack* _vars)
			: BTBehaviourContext(_errorReporter, _worldData, _vars)
			, lastResult(eBTResult::Undefined)
			, currBehaviourSpec(nullptr)
			, currBehaviourExec(nullptr)
		{}

		size_t getSeqIndex(const BTNode* node);
		void setSeqIndex(const BTNode* node, size_t idx);
	};

	inline size_t BTEvalContext::getSeqIndex(const BTNode* node)
	{
		auto it = seqIndex.find(node);
		if (it == seqIndex.end())
		{
			return 0;
		}

		return it->second;
	}

	inline void BTEvalContext::setSeqIndex(const BTNode* node, size_t idx)
	{
		seqIndex[node] = idx;
	}


	/* 
	 * BTTreeRuntimeData
	 */

	void BTTreeRuntimeData::compileExpressions(BTEvalContext& context)
	{
		treeRoot->compileExpressions(context);
	}


	/*
	 * BTEvalEngine
	 */

	BTEvalEngine::BTEvalEngine(BTTreeRuntimeData* _rtData, BTWorldData* worldData, VariablePack* vars)
		: rtData(_rtData)
	{
		assert(rtData);
		evalContext = new BTEvalContext(&errorReporter, worldData, vars);

		rtData->compileExpressions(*evalContext);
	}

	void BTEvalEngine::evaluate()
	{
		rtData->getTreeRoot()->evaluate(*evalContext);
	}


	/*
	 * BTCompositeNode
	 */

	BTCompositeNode::BTCompositeNode(const char* nodeName, std::initializer_list<BTNode*> children)
		: BTNode(nodeName)
		, childNodes(children)
	{}

	BTCompositeNode::~BTCompositeNode()
	{
		for (BTNode* node : childNodes)
		{
			node->~BTNode();
		}
	}

	void BTCompositeNode::compileExpressions(BTEvalContext& context)
	{
		for (BTNode* node : childNodes)
		{
			node->compileExpressions(context);
		}
	}

	void BTCompositeNode::addChildNode(BTNode* child)
	{
		childNodes.push_back(child);
	}


	/*
	 * BTConditionNode
	 */

	BTConditionNode::BTConditionNode(const char* nodeName, const char* _conditionText)
		: BTLeafNode(nodeName)
		, conditionText(_conditionText)
		, expData(nullptr)
	{}

	BTConditionNode::~BTConditionNode()
	{
		if (expData)
		{
			delete expData;
		}
	}

	void BTConditionNode::compileExpressions(BTEvalContext& context)
	{
		ExpressionCompiler comp(context.vars->getLayout());
		expData = comp.compile(conditionText);

		if (comp.errors().errorCount() > 0)
		{
			context.errorReporter->combine(comp.errors());
		}
		else if (expData->resultType != eExpType::BOOL)
		{
			context.errorReporter->addError(eBTErrorCategory::ExpressionType, eBTErrorCode::ConditionTypeNotBool, "Condition node expressions must be a boolean type");
		}
	}

	void BTConditionNode::evaluate(BTEvalContext& context) const
	{
		ExpressionEvaluator eval(context.vars);
		eval.evaluate(expData);

		if (eval.errors().errorCount() > 0)
		{
			context.lastResult = eBTResult::Failure;
			context.errorReporter->combine(eval.errors());
		}
		else
		{
			context.lastResult = eval.getBoolResult() ? eBTResult::Success : eBTResult::Failure;
		}
	}

	
	/*
	 * BTBehaviourNode
	 */

	BTBehaviourNode::BTBehaviourNode(const char* nodeName, BTBehaviourSpec* _behaviourSpec)
		: BTLeafNode(nodeName)
		, behaviourSpec(_behaviourSpec)
	{
		assert(behaviourSpec != nullptr);
	}

	void BTBehaviourNode::compileExpressions(BTEvalContext& context)
	{
		behaviourSpec->compileExpressions(context);
	}

	void BTBehaviourNode::evaluate(BTEvalContext& context) const
	{
		if (context.currBehaviourSpec != behaviourSpec.get())
		{
			// see if there's an existing behaviour running and stop it
			if (context.currBehaviourExec != nullptr)
			{
				context.currBehaviourExec->cleanUp(context);
				delete context.currBehaviourExec;
				context.currBehaviourExec = nullptr;
			}

			// start this behaviour
			context.currBehaviourSpec = behaviourSpec.get();
			context.currBehaviourExec = behaviourSpec->getNewExec(this, context);
			assert(context.currBehaviourExec);
			context.currBehaviourExec->init(this, context);
		}

		eBTResult res = context.currBehaviourExec->execute(context);
		context.lastResult = res;

		assert(res != eBTResult::Undefined);

		if (res != eBTResult::InProgress)
		{
			context.currBehaviourExec->cleanUp(context);
			delete context.currBehaviourExec;
			context.currBehaviourExec = nullptr;
			context.currBehaviourSpec = nullptr;
		}
	}


	/* 
	 * BTSequenceNode
	 */

	void BTSequenceNode::evaluate(BTEvalContext& context) const
	{
		for (size_t idx = context.getSeqIndex(this); idx < childNodes.size(); ++idx)
		{
			context.lastResult = eBTResult::Undefined;
			childNodes[idx]->evaluate(context);

			assert(context.lastResult != eBTResult::Undefined);

			if (context.lastResult != eBTResult::Success)
			{
				context.setSeqIndex(this, context.lastResult == eBTResult::InProgress ? idx : 0);
				return;
			}
		}

		context.lastResult = eBTResult::Success;
		context.setSeqIndex(this, 0);
	}


	/* 
	 * BTSelectorNode
	 */

	void BTSelectorNode::evaluate(BTEvalContext& context) const
	{
		for (size_t idx = 0; idx < childNodes.size(); ++idx)
		{
			context.lastResult = eBTResult::Undefined;
			childNodes[idx]->evaluate(context);

			assert(context.lastResult != eBTResult::Undefined);

			if (context.lastResult != eBTResult::Failure) return;
		}

		context.lastResult = eBTResult::Failure;
	}


} // namespace BehaviousTreeOO