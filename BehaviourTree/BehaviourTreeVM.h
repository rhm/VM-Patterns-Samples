/*
 * Behaviour tree system implemented as a virtual machine.
 */
#pragma once

#include <initializer_list>
#include <vector>
#include <memory>

#include "Expression.h"
#include "BTErrorReporter.h"


namespace BehaviourTreeVM
{
	enum class eBTResult : uint8_t
	{
		Undefined,

		Success,
		Failure,
		InProgress,
	};

	typedef uint16_t NodeIdx_t;

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

	class BTCompilerContext;
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

		virtual void compile(BTCompilerContext& context) const = 0;

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
		std::vector<std::unique_ptr<BTNode>> childNodes;

	public:
		BTCompositeNode(const char* nodeName, std::initializer_list<BTNode*> children);
		virtual ~BTCompositeNode() {};

		void addChildNode(BTNode* child);
	};


	class BTConditionNode : public BTLeafNode
	{
		const char* conditionText;

	public:
		BTConditionNode(const char* nodeName, const char *conditionText);

		virtual void compile(BTCompilerContext& context) const override;
	};


	class BTBehaviourNode : public BTLeafNode
	{
		std::unique_ptr<BTBehaviourSpec> behaviourSpec;

	public:
		BTBehaviourNode(const char* nodeName, BTBehaviourSpec* behaviourSpec);

		virtual void compile(BTCompilerContext& context) const override;

		const BTBehaviourSpec* getBehaviourSpec() const { return behaviourSpec.get(); }
	};
	

	class BTSequenceNode : public BTCompositeNode
	{
	public:
		BTSequenceNode(const char* nodeName, std::initializer_list<BTNode*> children)
			: BTCompositeNode(nodeName, children)
		{}

		virtual void compile(BTCompilerContext& context) const override;
	};


	class BTSelectorNode : public BTCompositeNode
	{
	public:
		BTSelectorNode(const char* nodeName, std::initializer_list<BTNode*> children)
			: BTCompositeNode(nodeName, children)
		{}

		virtual void compile(BTCompilerContext& context) const override;
	};


	/*
	 * Behaviours
	 */
	
	class BTBehaviourExec
	{
	public:
		// will be called once before execute() is called
		virtual void init(Name originNodeName, BTBehaviourContext& context) {};
		// called on each BT evaulation where the behaviour is executing, including the first
		virtual eBTResult execute(BTBehaviourContext& context) = 0;
		// called to clean up a behaviour that has stopped or is being interrupted
		virtual void cleanUp(BTBehaviourContext& context) {};
	};

	class BTBehaviourSpec
	{
	public:
		virtual BTBehaviourSpec* duplicate() const = 0;
		virtual void compileExpressions(BTBehaviourContext& context) {};
		virtual BTBehaviourExec* getNewExec(Name originNodeName, BTBehaviourContext& context) const = 0;
	};


	/*
	 * Behaviour tree pack
	 */

	class BTRuntimeData
	{
		friend class BTCompiler;
		friend class BTCompilerContext;
		friend class BTEvalEngine;

		const VariableLayout* variableLayout;
		NodeIdx_t seqNodeCount;
		std::vector<uint32_t> byteCode;
		std::vector<std::unique_ptr<ExpressionData>> expData;
		std::vector<Name> nodeNames;
		std::vector<std::unique_ptr<BTBehaviourSpec>> behaviourSpecs;

		BTRuntimeData();
	};
	

	/*
	 * Behaviour tree compiler
	 */

	class BTCompiler
	{
		BTErrorReporter errorReport;
		const VariablePack* vars;
		BTWorldData* worldData;

	public:
		BTCompiler(const VariablePack* _vars, BTWorldData* _worldData);

		BTRuntimeData* compile(const BTNode* rootNode);
		const BTErrorReporter& errors() const { return errorReport; }
	};

	
	/*
	 * Behaviour tree engine
	 */

	class BTEvalEngine
	{
		BTErrorReporter errorReporter;

		BTRuntimeData* rtData;
		BTBehaviourContext context;

		Name currNodeName;
		NodeIdx_t currBehaviourIdx;
		BTBehaviourExec* currBehaviourExec;
		std::vector<NodeIdx_t> seqCounters;

		static const NodeIdx_t invalidBehaviourIdx = UINT16_MAX;

	public:
		BTEvalEngine(BTRuntimeData* _rtData, BTWorldData* worldData, VariablePack* vars);

		void evaluate();

		const BTErrorReporter& errors() const { return errorReporter; }
	};


} // namespace BehaviousTreeOO
