/*
 * Behaviour tree system implemented in an OO style, except for the formula evaluation, which uses the Expression VM.
 */
#pragma once

#include <initializer_list>
#include <vector>
#include <memory>

#include "Expression.h"
#include "BTErrorReporter.h"


namespace BehaviourTreeOO
{
	enum class eBTResult : uint8_t
	{
		Undefined,

		Success,
		Failure,
		InProgress,
	};

	class BTWorldData
	{};

	struct BTBehaviourContext
	{
		BTErrorReporter* errorReporter;
		BTWorldData* worldData;
		const VariablePack* vars;

		BTBehaviourContext(BTErrorReporter* _errorReporter, BTWorldData* _worldData, const VariablePack* _vars)
			: errorReporter(_errorReporter)
			, worldData(_worldData)
			, vars(_vars)
		{}
	};
	struct BTEvalContext;

	class BTBehaviourSpec;


	/*
	 * Behaviour tree nodes
	 */

	class BTNode
	{
		const char *nodeName;

	public:
		BTNode(const char* _nodeName)
			: nodeName(_nodeName)
		{}
		virtual ~BTNode() {};

		virtual void compileExpressions(BTEvalContext& context) = 0;
		virtual void evaluate(BTEvalContext& context) const = 0;

		const char* getNodeName() const { return nodeName; }
	};


	class BTLeafNode : public BTNode
	{
	public:
		BTLeafNode(const char* nodeName)
			: BTNode(nodeName)
		{}
	};


	class BTCompositeNode : public BTNode
	{
	protected:
		std::vector<BTNode*> childNodes;

	public:
		BTCompositeNode(const char* nodeName, std::initializer_list<BTNode*> children);
		virtual ~BTCompositeNode();

		virtual void compileExpressions(BTEvalContext& context) override;

		void addChildNode(BTNode* child);
	};


	class BTConditionNode : public BTLeafNode
	{
		const char* conditionText;
		ExpressionData *expData;

	public:
		BTConditionNode(const char* nodeName, const char *conditionText);
		virtual ~BTConditionNode();

		virtual void compileExpressions(BTEvalContext& context) override;
		virtual void evaluate(BTEvalContext& context) const override;	
	};


	class BTBehaviourNode : public BTLeafNode
	{
		std::unique_ptr<BTBehaviourSpec> behaviourSpec;

	public:
		BTBehaviourNode(const char* nodeName, BTBehaviourSpec* behaviourSpec);

		virtual void compileExpressions(BTEvalContext& context) override;
		virtual void evaluate(BTEvalContext& context) const override;

		const BTBehaviourSpec* getBehaviourSpec() const { return behaviourSpec.get(); }
	};
	

	class BTSequenceNode : public BTCompositeNode
	{
	public:
		BTSequenceNode(const char* nodeName, std::initializer_list<BTNode*> children)
			: BTCompositeNode(nodeName, children)
		{}

		virtual void evaluate(BTEvalContext& context) const override;
	};


	class BTSelectorNode : public BTCompositeNode
	{
	public:
		BTSelectorNode(const char* nodeName, std::initializer_list<BTNode*> children)
			: BTCompositeNode(nodeName, children)
		{}

		virtual void evaluate(BTEvalContext& context) const override;
	};


	/*
	 * Behaviours
	 */


	class BTBehaviourExec
	{
	public:
		// will be called once before execute() is called
		virtual void init(const BTBehaviourNode* originNode, BTBehaviourContext& context) {};
		// called on each BT evaulation where the behaviour is executing, including the first
		virtual eBTResult execute(BTBehaviourContext& context) = 0;
		// called to clean up a behaviour that has stopped or is being interrupted
		virtual void cleanUp(BTBehaviourContext& context) {};
	};

	class BTBehaviourSpec
	{
	public:
		virtual void compileExpressions(BTBehaviourContext& context) {};
		virtual BTBehaviourExec* getNewExec(Name originNodeName, BTBehaviourContext& context) const = 0;
	};


	/*
	 * Behaviour tree pack
	 */

	class BTTreeRuntimeData
	{
		const VariableLayout* variableLayout;
		std::unique_ptr<BTNode> treeRoot;

	public:
		BTTreeRuntimeData(const VariableLayout* layout, BTNode* root)
			: variableLayout(layout)
			, treeRoot(root)
		{}

		void compileExpressions(BTEvalContext& context);

		const BTNode* getTreeRoot() const { return treeRoot.get(); }
		const VariableLayout* getVariableLayout() const { return variableLayout; }
	};
	

	/*
	 * Behaviour tree engine
	 */

	class BTEvalEngine
	{
		BTErrorReporter errorReporter;

		BTTreeRuntimeData* rtData;
		BTEvalContext* evalContext;

	public:
		BTEvalEngine(BTTreeRuntimeData* _rtData, BTWorldData* worldData, VariablePack* vars);

		void evaluate();

		const BTErrorReporter& errors() const { return errorReporter; }
	};


} // namespace BehaviousTreeOO
