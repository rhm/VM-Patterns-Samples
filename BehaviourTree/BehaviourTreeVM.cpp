/*
 * Behaviour tree system implemented in an OO style, except for the formula evaluation, which uses the Expression VM.
 */

#include "stdafx.h"

#include <unordered_map>
#include <iostream>
#include <iomanip>

#include "BehaviourTreeVM.h"

#define DEBUG_PRINT 1

namespace BehaviourTreeVM
{

	enum class eBTOpcode : uint16_t
	{
		INVALID,

		INDICATE_NODE_START,

		SET_FAIL,
		SET_SUCCESS,

		STORE_SEQIDX,
		COND_STORE_SEQIDX,

		EVAL_EXPR,
		EXEC_BEHAVIOUR,

		JUMP,
		JUMP_TABLE,
		JUMP_NOT_FAIL,
		JUMP_NOT_SUCCESS,

		MAX,
	};


	/*
	 * BTRuntimeData
	 */

	BTRuntimeData::BTRuntimeData()
		: variableLayout(nullptr)
		, seqNodeCount(0)
	{}


	/*
	 * BTCompilerContext
	 */

	class BTCompilerContext
	{
		BTErrorReporter* errorReport;
		BTRuntimeData* rtData;

		BTBehaviourContext* behaviourContext;

		struct FixUp
		{
			NodeIdx_t address;
			int label;
			bool highHalf;

			FixUp(NodeIdx_t _address, bool _highHalf, NodeIdx_t _label)
				: address(_address)
				, label(_label)
				, highHalf(_highHalf)
			{}
		};

		std::vector<FixUp> fixups;
		std::unordered_map<int, NodeIdx_t> labels;

		int nextLabel;

	public:
		static const NodeIdx_t invalidAddress = 0xcdcd;

		BTCompilerContext(BTErrorReporter* _errorReport, BTBehaviourContext* _behaviourContext);
		
		BTErrorReporter& errors() { return *errorReport; }
		BTBehaviourContext* getBehaviourContext() { return behaviourContext; }

		int allocateLabel();
		void emitLabel(int label);
		void recordFixup(NodeIdx_t address, bool highHalf, int label);

		NodeIdx_t emitOpcode(eBTOpcode opcode, NodeIdx_t operand);
		NodeIdx_t emitOpcode(eBTOpcode opcode, NodeIdx_t operandA, NodeIdx_t operandB);
		NodeIdx_t emitData(NodeIdx_t high, NodeIdx_t low);

		NodeIdx_t storeExpressionData(ExpressionData* expData);
		NodeIdx_t storeNodeName(Name name);
		NodeIdx_t storeBehaviourSpec(BTBehaviourSpec* behaviourSpec);
		NodeIdx_t incrementSeqNodeCount();

		void fixupLabels();

#if DEBUG_PRINT
		void debugDumpBytes();
		void debugDumpFixups();
#endif

		BTRuntimeData* getRuntimeData();
	};

	BTCompilerContext::BTCompilerContext(BTErrorReporter* _errorReport, BTBehaviourContext* _behaviourContext)
		: errorReport(_errorReport)
		, behaviourContext(_behaviourContext)
	{
		rtData = new BTRuntimeData();
		nextLabel = 0;
	}

	int BTCompilerContext::allocateLabel()
	{
		return nextLabel++;
	}

	void BTCompilerContext::emitLabel(int label)
	{
		labels[label] = rtData->byteCode.size();
	}

	void BTCompilerContext::recordFixup(NodeIdx_t address, bool highHalf, int label)
	{
		fixups.emplace_back(address, highHalf, label);
	}

	NodeIdx_t BTCompilerContext::emitOpcode(eBTOpcode opcode, NodeIdx_t operand)
	{
		uint32_t word = (static_cast<uint16_t>(opcode) | (operand << 16));
		rtData->byteCode.push_back(word);

		return rtData->byteCode.size()-1;
	}

	NodeIdx_t BTCompilerContext::emitOpcode(eBTOpcode opcode, NodeIdx_t operandA, NodeIdx_t operandB)
	{
		uint32_t word = (static_cast<uint16_t>(opcode) | (operandA << 16));
		rtData->byteCode.push_back(word);
		rtData->byteCode.push_back(operandB);

		return rtData->byteCode.size()-2;
	}

	NodeIdx_t BTCompilerContext::emitData(NodeIdx_t high, NodeIdx_t low)
	{
		uint32_t word = (high << 16) | (low);
		rtData->byteCode.push_back(word);

		return rtData->byteCode.size()-1;
	}

	NodeIdx_t BTCompilerContext::storeExpressionData(ExpressionData* expData)
	{
		rtData->expData.emplace_back(expData);
		return rtData->expData.size()-1;
	}

	NodeIdx_t BTCompilerContext::storeNodeName(Name name)
	{
		rtData->nodeNames.push_back(name);
		return rtData->nodeNames.size()-1;
	}

	NodeIdx_t BTCompilerContext::storeBehaviourSpec(BTBehaviourSpec* behaviourSpec)
	{
		rtData->behaviourSpecs.emplace_back(behaviourSpec);
		return rtData->behaviourSpecs.size()-1;	
	}

	NodeIdx_t BTCompilerContext::incrementSeqNodeCount()
	{
		NodeIdx_t currCount = rtData->seqNodeCount;
		rtData->seqNodeCount += 1;
		return currCount;
	}

	void BTCompilerContext::fixupLabels()
	{
		for (const auto& fixup : fixups)
		{
			assert(labels.find(fixup.label) != labels.end());

			NodeIdx_t labelAddress = labels[fixup.label];
			uint32_t word = rtData->byteCode[fixup.address];

			if (fixup.highHalf)
			{
				word = (labelAddress << 16) | (word & 0xffff);
			}
			else
			{
				word = (word & 0xffff0000) | labelAddress;
			}

			rtData->byteCode[fixup.address] = word;
		}
	}

#if DEBUG_PRINT
	void BTCompilerContext::debugDumpBytes()
	{
		using namespace std;

		cout << "addr high   low  instr" << endl;
		cout << "---- ----- ----- -----" << endl;

		auto print_line = [](size_t ip, uint32_t word, initializer_list<const char*> list)
		{
			cout << setw(4) << ip << " " << setw(5) << (word >> 16) << " " << setw(5) << (word & 0xffff) << " ";
			for (const char *str : list)
			{
				cout << str << " ";
			}
			cout << endl;
		};

		const size_t codeLen(rtData->byteCode.size());			
		for (size_t ip = 0; ip < codeLen; ++ip)
		{
			const uint32_t word(rtData->byteCode[ip]);
			eBTOpcode opcode = static_cast<eBTOpcode>(word & 0xffff);
			NodeIdx_t operand = (word >> 16);

			switch(opcode)
			{
			case eBTOpcode::INDICATE_NODE_START:
				print_line(ip, word, { "INDICATE_NODE_START", rtData->nodeNames[operand].c_str() });
				break;

			case eBTOpcode::SET_FAIL:
				print_line(ip, word, { "SET_FAIL" });
				break;

			case eBTOpcode::SET_SUCCESS:
				print_line(ip, word, { "SET_SUCCESS" });
				break;

			case eBTOpcode::STORE_SEQIDX:
				print_line(ip, word, { "STORE_SEQIDX" });
				++ip;
				print_line(ip, rtData->byteCode[ip], {"value to store"});
				break;

			case eBTOpcode::COND_STORE_SEQIDX:
				print_line(ip, word, { "COND_STORE_SEQIDX" });
				++ip;
				print_line(ip, rtData->byteCode[ip], {"value to store if result=InProgress"});
				break;

			case eBTOpcode::EVAL_EXPR:
				print_line(ip, word, {"EVAIL_EXPR"});
				break;

			case eBTOpcode::EXEC_BEHAVIOUR:
				print_line(ip, word, {"EXEC_BEHAVIOUR"});
				break;

			case eBTOpcode::JUMP_TABLE:
				{
					print_line(ip, word, { "JUMP_TABLE" });
					++ip;
					uint32_t cnt = rtData->byteCode[ip];
					print_line(ip, cnt, { "table len (only present if DEBUG_PRINT=1)" });
					for (uint32_t i = 0; i < cnt; ++i)
					{
						++ip;
						print_line(ip, rtData->byteCode[ip], {"jump target"});
					}
				}
				break;

			case eBTOpcode::JUMP_NOT_FAIL:
				print_line(ip, word, {"JUMP_NOT_FAIL"});
				break;

			case eBTOpcode::JUMP_NOT_SUCCESS:
				print_line(ip, word, {"JUMP_NOT_SUCCESS"});
				break;

			default:
				assert(false);
				break;
			}
		}
	}

	void BTCompilerContext::debugDumpFixups()
	{
		using namespace std;

		cout << endl;

		cout << "Label Addr" << endl;
		cout << "----- ----" << endl;

		for (const auto& label : labels)
		{
			cout << setw(5) << label.first << " " << setw(4) << label.second << endl;
		}

		cout << endl;

		cout << "Fixup addr high label" << endl;
		cout << "---------- ---- -----" << endl;

		for (const auto& fixup : fixups)
		{
			cout << setw(10) << fixup.address << (fixup.highHalf? " h    ":" l    ") << setw(5) << fixup.label << endl;
		}
	}
#endif // DEBUG_PRINT

	BTRuntimeData* BTCompilerContext::getRuntimeData()
	{
		BTRuntimeData* retval = rtData;
		rtData = nullptr;
		return retval;
	}
	
	
	/*
	 * BTCompositeNode
	 */

	BTCompositeNode::BTCompositeNode(const char* nodeName, std::initializer_list<BTNode*> children)
		: BTNode(nodeName)
	{
		for (BTNode* node : children)
		{
			childNodes.emplace_back(node);
		}
	}

	/*
	 * BTConditionNode
	 */

	BTConditionNode::BTConditionNode(const char* nodeName, const char *_conditionText)
		: BTLeafNode(nodeName)
		, conditionText(_conditionText)
	{}

	void BTConditionNode::compile(BTCompilerContext& context) const
	{
		ExpressionCompiler comp(context.getBehaviourContext()->vars->getLayout());
		ExpressionData *exprData = comp.compile(conditionText);

		if (comp.errors().errorCount() > 0)
		{
			context.errors().combine(comp.errors());
		}
		else if (exprData->resultType != eExpType::BOOL)
		{
			context.errors().addError(eBTErrorCategory::ExpressionType, eBTErrorCode::ConditionTypeNotBool, "Condition node expressions must be a boolean type");
		}
		else
		{
			assert(exprData);

			NodeIdx_t exprIdx = context.storeExpressionData(exprData);
			context.emitOpcode(eBTOpcode::EVAL_EXPR, exprIdx);
		}
	}


	/*
	 * BTBehaviourNode
	 */

	BTBehaviourNode::BTBehaviourNode(const char* nodeName, BTBehaviourSpec* _behaviourSpec)
		: BTLeafNode(nodeName)
		, behaviourSpec(_behaviourSpec)
	{}

	void BTBehaviourNode::compile(BTCompilerContext& context) const
	{
		BTBehaviourSpec *copyBehaviourSpec = behaviourSpec->duplicate();
		assert(copyBehaviourSpec);

		copyBehaviourSpec->compileExpressions(*context.getBehaviourContext());

		NodeIdx_t behaviourIdx = context.storeBehaviourSpec(copyBehaviourSpec);
		NodeIdx_t nameIdx = context.storeNodeName(Name(getNodeName()));

		context.emitOpcode(eBTOpcode::INDICATE_NODE_START, nameIdx);
		context.emitOpcode(eBTOpcode::EXEC_BEHAVIOUR, behaviourIdx);
	}


	/*
	 * BTSequenceNode
	 */

	void BTSequenceNode::compile(BTCompilerContext& context) const
	{
		NodeIdx_t seqIdx = context.incrementSeqNodeCount();
		int endLabel = context.allocateLabel();

		// emit the jumptable for resuming sequences
#if DEBUG_PRINT
		context.emitOpcode(eBTOpcode::JUMP_TABLE, seqIdx, childNodes.size());
#else
		context.emitOpcode(eBTOpcode::JUMP_TABLE, seqIdx);
#endif
		std::vector<int> jumpTableLabels;
		for (size_t idx = 0; idx < childNodes.size(); ++idx)
		{
			int label = context.allocateLabel();
			jumpTableLabels.push_back(label);

			NodeIdx_t addr = context.emitData(BTCompilerContext::invalidAddress, BTCompilerContext::invalidAddress);
			context.recordFixup(addr, false, label);
		}

		// emit each subnode
		for (size_t idx = 0; idx < childNodes.size(); ++idx)
		{
			context.emitLabel(jumpTableLabels[idx]);
			childNodes[idx]->compile(context);

			context.emitOpcode(eBTOpcode::COND_STORE_SEQIDX, seqIdx, idx);
			NodeIdx_t jumpInstr = context.emitOpcode(eBTOpcode::JUMP_NOT_SUCCESS, BTCompilerContext::invalidAddress);
			context.recordFixup(jumpInstr, true, endLabel);
		}

		// this will be skipped if any sub-node returns FAIL or IN_PROGRESS
		context.emitOpcode(eBTOpcode::STORE_SEQIDX, seqIdx, 0);
		context.emitOpcode(eBTOpcode::SET_SUCCESS, 0);

		context.emitLabel(endLabel);
	}
	

	/*
	 * BTSelectorNode
	 */

	void BTSelectorNode::compile(BTCompilerContext& context) const
	{
		int endLabel = context.allocateLabel();

		for (size_t idx = 0; idx < childNodes.size(); ++idx)
		{
			childNodes[idx]->compile(context);
			
			NodeIdx_t jumpInstr = context.emitOpcode(eBTOpcode::JUMP_NOT_FAIL, BTCompilerContext::invalidAddress);
			context.recordFixup(jumpInstr, true, endLabel);
		}

		context.emitLabel(endLabel);
	}
	

	/*
	 * BTCompiler
	 */

	BTCompiler::BTCompiler(const VariablePack* _vars, BTWorldData* _worldData)
		: vars(_vars)
		, worldData(_worldData)
	{}

	BTRuntimeData* BTCompiler::compile(const BTNode* rootNode)
	{
		BTBehaviourContext behaviourContext(&errorReport, worldData, vars);
		BTCompilerContext compilerContext(&errorReport, &behaviourContext);

		rootNode->compile(compilerContext);
	
		if (errorReport.errorCount() == 0)
		{
			compilerContext.debugDumpBytes();
			compilerContext.debugDumpFixups();

			compilerContext.fixupLabels();

			compilerContext.debugDumpBytes();

			return compilerContext.getRuntimeData();
		}
		else
		{
			return nullptr;
		}
	}
	

	/*
	 * BTEvalEngine
	 */

	BTEvalEngine::BTEvalEngine(BTRuntimeData* _rtData, BTWorldData* worldData, VariablePack* vars)
		: rtData(_rtData)
		, context(&errorReporter, worldData, vars)
		, currBehaviourIdx(invalidBehaviourIdx)
		, currBehaviourExec(nullptr)
	{
		seqCounters.resize(rtData->seqNodeCount, 0);
	}

	void BTEvalEngine::evaluate()
	{
		errorReporter.reset();
		
		eBTResult result = eBTResult::Undefined;
		ExpressionEvaluator expEval(context.vars);

		const size_t codeLen(rtData->byteCode.size());
			
		for (size_t ip = 0; ip < codeLen; ++ip)
		{
			const uint32_t word(rtData->byteCode[ip]);
			eBTOpcode opcode = static_cast<eBTOpcode>(word & 0xffff);
			NodeIdx_t operand = (word >> 16);
			
			switch(opcode)
			{
			case eBTOpcode::INDICATE_NODE_START:
				currNodeName = rtData->nodeNames[operand];
				break;

			case eBTOpcode::SET_FAIL:
				result = eBTResult::Failure;
				break;

			case eBTOpcode::SET_SUCCESS:
				result = eBTResult::Success;
				break;

			case eBTOpcode::STORE_SEQIDX:
				{
					NodeIdx_t operandB = (rtData->byteCode[++ip] & 0xffff);
					seqCounters[operand] = operandB;
				}
				break;

			case eBTOpcode::COND_STORE_SEQIDX:
				{
					NodeIdx_t operandB = (rtData->byteCode[++ip] & 0xffff);
					seqCounters[operand] = result == eBTResult::InProgress ? operandB : 0;
				}
				break;

			case eBTOpcode::EVAL_EXPR:
				{
					expEval.reset();
					expEval.evaluate(rtData->expData[operand].get());

					if (expEval.errors().errorCount() > 0)
					{
						result = eBTResult::Failure;
						errorReporter.combine(expEval.errors());
					}
					else
					{
						result = expEval.getBoolResult() ? eBTResult::Success : eBTResult::Failure;
					}
				}
				break;

			case eBTOpcode::EXEC_BEHAVIOUR:
				{
					if (currBehaviourIdx != operand)
					{
						// see if there's an existing behaviour running and stop it
						if (currBehaviourExec != nullptr)
						{
							currBehaviourExec->cleanUp(context);
							delete currBehaviourExec;
							currBehaviourExec = nullptr;
						}

						// start this behaviour
						currBehaviourIdx = operand;
						const BTBehaviourSpec* spec = rtData->behaviourSpecs[operand].get();
						currBehaviourExec = spec->getNewExec(currNodeName, context);
						assert(currBehaviourExec);
						currBehaviourExec->init(currNodeName, context);
					}

					result = currBehaviourExec->execute(context);
					assert(result != eBTResult::Undefined);

					if (result != eBTResult::InProgress)
					{
						currBehaviourExec->cleanUp(context);
						delete currBehaviourExec;
						currBehaviourExec = nullptr;
						currBehaviourIdx = invalidBehaviourIdx;
					}
				}
				break;

			case eBTOpcode::JUMP_TABLE:
				{
					NodeIdx_t counter = seqCounters[operand];
#if DEBUG_PRINT
					ip = (rtData->byteCode[ip + 2 + counter] & 0xffff) - 1;
#else
					ip = (rtData->byteCode[ip + 1 + counter] & 0xffff) - 1; // -1 to account for loop increment
#endif
				}
				break;

			case eBTOpcode::JUMP_NOT_FAIL:
				if (result != eBTResult::Failure)
				{
					ip = operand-1; // -1 to account for loop increment
				}
				break;

			case eBTOpcode::JUMP_NOT_SUCCESS:
				if (result != eBTResult::Success)
				{
					ip = operand-1; // -1 to account for loop increment
				}
				break;

			default:
				assert(false);
				break;
			}
		}
	}
	

} // namepsace BehaviourTreeVM
