/*
 * Expression.cpp
 *
 * AST nodes, compiler core, VM definition and execution engine. Ideally this would be split out
 * into more files, but to keep the projects small for this example, it is all in one.
 *
 */

#include "stdafx.h"

#include <sstream>
#include <stdlib.h>
#include <math.h>

#include "Expression.h"
#include "Name.h"


/*
 * ResultInfo - describes where the results of a node come from
 */

enum class eResultSource
{
	INVALID,

	Constant,
	Register,
	Variable
};

struct ResultInfo
{
	eResultSource source;
	ExpressionSlotIndex index;

	ResultInfo(eResultSource _source, ExpressionSlotIndex _index)
		: source(_source)
		, index(_index)
	{}
};


/*
 * Utility functions
 *
 */

const char *getTypeAsString(eExpType type)
{
	switch (type)
	{
	case eExpType::BOOL:
		return "BOOL";

	case eExpType::NAME:
		return "NAME";

	case eExpType::NUMBER:
		return "NUMBER";

	default:
		return "!ERROR!";
	}
}


/* 
 * Bytecode values
 */

#define LEFT_REG_BITS    0x00
#define LEFT_CONST_BITS  0x04 // 0b00000100
#define LEFT_VAR_BITS    0x08 // 0b00001000
#define RIGHT_REG_BITS   0x00
#define RIGHT_CONST_BITS 0x01 // 0b00000001
#define RIGHT_VAR_BITS   0x02 // 0b00000010

#define OP_FLAG_BITS 4
#define OPCODE(OP,LEFT,RIGHT) ((((uint8_t)OP)<<(OP_FLAG_BITS))|(LEFT)|(RIGHT))


enum class eSimpleOp : uint8_t
{
	UNINITIALISED,

	ADD,
	SUB,
	MUL,
	DIV,
	MOD,

	AND,
	OR,
	XOR,
	NOT,

	NAME_EQ,
	NAME_NEQ,
	BOOL_EQ,
	NUM_EQ,
	NUM_NEQ,
	NUM_LT,
	NUM_GT,
	NUM_LTEQ,
	NUM_GTEQ,

	NUM_VAL,
	BOOL_VAL
};


enum class eEncOpcode : uint16_t
{
	UNINITIALISED = eSimpleOp::UNINITIALISED,

	// Arithmetic (Numeric)
	ADD			= OPCODE(eSimpleOp::ADD,LEFT_REG_BITS,  RIGHT_REG_BITS),
	ADD_LC		= OPCODE(eSimpleOp::ADD,LEFT_CONST_BITS,RIGHT_REG_BITS),
	ADD_LV		= OPCODE(eSimpleOp::ADD,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	ADD_LV_RV	= OPCODE(eSimpleOp::ADD,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	ADD_LC_RV   = OPCODE(eSimpleOp::ADD,LEFT_CONST_BITS,RIGHT_VAR_BITS),

	SUB			= OPCODE(eSimpleOp::SUB,LEFT_REG_BITS,  RIGHT_REG_BITS),
	SUB_LC		= OPCODE(eSimpleOp::SUB,LEFT_CONST_BITS,RIGHT_REG_BITS),
	SUB_LV		= OPCODE(eSimpleOp::SUB,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	SUB_RC		= OPCODE(eSimpleOp::SUB,LEFT_REG_BITS,  RIGHT_CONST_BITS),
	SUB_RV		= OPCODE(eSimpleOp::SUB,LEFT_REG_BITS,  RIGHT_VAR_BITS),
	SUB_LC_RV	= OPCODE(eSimpleOp::SUB,LEFT_CONST_BITS,RIGHT_VAR_BITS),
	SUB_LV_RC	= OPCODE(eSimpleOp::SUB,LEFT_VAR_BITS,  RIGHT_CONST_BITS),
	SUB_LV_RV	= OPCODE(eSimpleOp::SUB,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	
	MUL			= OPCODE(eSimpleOp::MUL,LEFT_REG_BITS,  RIGHT_REG_BITS),
	MUL_LC		= OPCODE(eSimpleOp::MUL,LEFT_CONST_BITS,RIGHT_REG_BITS),
	MUL_LV		= OPCODE(eSimpleOp::MUL,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	MUL_LV_RV	= OPCODE(eSimpleOp::MUL,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	MUL_LC_RV	= OPCODE(eSimpleOp::MUL,LEFT_CONST_BITS,RIGHT_VAR_BITS),

	DIV			= OPCODE(eSimpleOp::DIV,LEFT_REG_BITS,  RIGHT_REG_BITS),
	DIV_LC		= OPCODE(eSimpleOp::DIV,LEFT_CONST_BITS,RIGHT_REG_BITS),
	DIV_LV		= OPCODE(eSimpleOp::DIV,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	DIV_RC		= OPCODE(eSimpleOp::DIV,LEFT_REG_BITS,  RIGHT_CONST_BITS),
	DIV_RV		= OPCODE(eSimpleOp::DIV,LEFT_REG_BITS,  RIGHT_VAR_BITS),
	DIV_LC_RV	= OPCODE(eSimpleOp::DIV,LEFT_CONST_BITS,RIGHT_VAR_BITS),
	DIV_LV_RC	= OPCODE(eSimpleOp::DIV,LEFT_VAR_BITS,  RIGHT_CONST_BITS),
	DIV_LV_RV	= OPCODE(eSimpleOp::DIV,LEFT_VAR_BITS,  RIGHT_VAR_BITS),

	MOD			= OPCODE(eSimpleOp::MOD,LEFT_REG_BITS,  RIGHT_REG_BITS),
	MOD_LC		= OPCODE(eSimpleOp::MOD,LEFT_CONST_BITS,RIGHT_REG_BITS),
	MOD_LV		= OPCODE(eSimpleOp::MOD,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	MOD_RC		= OPCODE(eSimpleOp::MOD,LEFT_REG_BITS,  RIGHT_CONST_BITS),
	MOD_RV		= OPCODE(eSimpleOp::MOD,LEFT_REG_BITS,  RIGHT_VAR_BITS),
	MOD_LC_RV	= OPCODE(eSimpleOp::MOD,LEFT_CONST_BITS,RIGHT_VAR_BITS),
	MOD_LV_RC	= OPCODE(eSimpleOp::MOD,LEFT_VAR_BITS,  RIGHT_CONST_BITS),
	MOD_LV_RV	= OPCODE(eSimpleOp::MOD,LEFT_VAR_BITS,  RIGHT_VAR_BITS),

	// Logic (Boolean)
	AND			= OPCODE(eSimpleOp::AND,LEFT_REG_BITS,  RIGHT_REG_BITS),
	OR			= OPCODE(eSimpleOp::OR,LEFT_REG_BITS,  RIGHT_REG_BITS),
	XOR			= OPCODE(eSimpleOp::XOR,LEFT_REG_BITS, RIGHT_REG_BITS),
	NOT			= OPCODE(eSimpleOp::NOT,LEFT_REG_BITS, RIGHT_REG_BITS), // right not used

	// Comparison (Names)
	NAME_EQ_LC_RV  = OPCODE(eSimpleOp::NAME_EQ ,LEFT_CONST_BITS,RIGHT_VAR_BITS),
	NAME_EQ_LV_RV  = OPCODE(eSimpleOp::NAME_EQ ,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NAME_NEQ_LC_RV = OPCODE(eSimpleOp::NAME_NEQ,LEFT_CONST_BITS,RIGHT_VAR_BITS),
	NAME_NEQ_LV_RV = OPCODE(eSimpleOp::NAME_NEQ,LEFT_VAR_BITS  ,RIGHT_VAR_BITS),

	// Comparison (Boolean)	[NEQ is handled by XOR]
	BOOL_EQ		  = OPCODE(eSimpleOp::BOOL_EQ ,LEFT_REG_BITS,RIGHT_REG_BITS),

	// Comparison (Numeric)
	NUM_EQ			= OPCODE(eSimpleOp::NUM_EQ,  LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_EQ_LC		= OPCODE(eSimpleOp::NUM_EQ,  LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_EQ_LV		= OPCODE(eSimpleOp::NUM_EQ,  LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_EQ_LV_RV	= OPCODE(eSimpleOp::NUM_EQ,  LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_EQ_LV_RC	= OPCODE(eSimpleOp::NUM_EQ,  LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	NUM_NEQ			= OPCODE(eSimpleOp::NUM_NEQ,  LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_NEQ_LC		= OPCODE(eSimpleOp::NUM_NEQ,  LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_NEQ_LV		= OPCODE(eSimpleOp::NUM_NEQ,  LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_NEQ_LV_RV	= OPCODE(eSimpleOp::NUM_NEQ,  LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_NEQ_LV_RC	= OPCODE(eSimpleOp::NUM_NEQ,  LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	NUM_LT			= OPCODE(eSimpleOp::NUM_LT,  LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_LT_LC		= OPCODE(eSimpleOp::NUM_LT,  LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_LT_LV		= OPCODE(eSimpleOp::NUM_LT,  LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_LT_LV_RV	= OPCODE(eSimpleOp::NUM_LT,  LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_LT_LV_RC	= OPCODE(eSimpleOp::NUM_LT,  LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	NUM_GT			= OPCODE(eSimpleOp::NUM_GT,  LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_GT_LC		= OPCODE(eSimpleOp::NUM_GT,  LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_GT_LV		= OPCODE(eSimpleOp::NUM_GT,  LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_GT_LV_RV	= OPCODE(eSimpleOp::NUM_GT,  LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_GT_LV_RC	= OPCODE(eSimpleOp::NUM_GT,  LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	NUM_LTEQ		= OPCODE(eSimpleOp::NUM_LTEQ,LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_LTEQ_LC		= OPCODE(eSimpleOp::NUM_LTEQ,LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_LTEQ_LV		= OPCODE(eSimpleOp::NUM_LTEQ,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_LTEQ_LV_RV	= OPCODE(eSimpleOp::NUM_LTEQ,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_LTEQ_LV_RC	= OPCODE(eSimpleOp::NUM_LTEQ,LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	NUM_GTEQ		= OPCODE(eSimpleOp::NUM_GTEQ,LEFT_REG_BITS,  RIGHT_REG_BITS),
	NUM_GTEQ_LC		= OPCODE(eSimpleOp::NUM_GTEQ,LEFT_CONST_BITS,RIGHT_REG_BITS),
	NUM_GTEQ_LV		= OPCODE(eSimpleOp::NUM_GTEQ,LEFT_VAR_BITS,  RIGHT_REG_BITS),
	NUM_GTEQ_LV_RV	= OPCODE(eSimpleOp::NUM_GTEQ,LEFT_VAR_BITS,  RIGHT_VAR_BITS),
	NUM_GTEQ_LV_RC	= OPCODE(eSimpleOp::NUM_GTEQ,LEFT_VAR_BITS,  RIGHT_CONST_BITS),

	// Value operations (for const expressions)
	NUM_VAL_LC		= OPCODE(eSimpleOp::NUM_VAL, LEFT_CONST_BITS,RIGHT_CONST_BITS),
	BOOL_VAL_LC     = OPCODE(eSimpleOp::BOOL_VAL,LEFT_CONST_BITS,RIGHT_CONST_BITS),

	OPCODE_MAX
};

inline eEncOpcode encodeOp(eSimpleOp simpleOp, eResultSource leftSource, eResultSource rightSource)
{
	uint8_t left, right;

	switch (leftSource)
	{
	case eResultSource::Register: left = LEFT_REG_BITS; break;
	case eResultSource::Constant: left = LEFT_CONST_BITS; break;
	case eResultSource::Variable: left = LEFT_VAR_BITS; break;
	default:
		assert(false); break;
	}

	switch (rightSource)
	{
	case eResultSource::Register: right = RIGHT_REG_BITS; break;
	case eResultSource::Constant: right = RIGHT_CONST_BITS; break;
	case eResultSource::Variable: right = RIGHT_VAR_BITS; break;
	default:
		assert(false); break;
	}

	return static_cast<eEncOpcode>(OPCODE(static_cast<uint8_t>(simpleOp), left, right));
}


/* 
 * ExpressionDataWriter
 */

class ExpressionDataWriter
{
	ExpressionData *data;

public:
	ExpressionDataWriter();
	~ExpressionDataWriter();

	ExpressionSlotIndex addNumericConst(float value);
	ExpressionSlotIndex addNameConst(Name value);

	void emitInstr(eEncOpcode opcode, ExpressionSlotIndex resultReg, ExpressionSlotIndex leftOperand, ExpressionSlotIndex rightOperand);

	ExpressionData *getData();
};


/*
 * Node classes
 *
 */


class ASTNode
{
	eASTNodeType NodeType;

protected:
	eExpType ExprType;

	ASTNode(eASTNodeType _nodeType)
		: NodeType(_nodeType)
	{}

	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter) { return true; }

public:
	virtual ~ASTNode() {};

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) = 0;
	virtual bool constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
	virtual void gatherConsts(ExpressionDataWriter& writer) {};
	virtual void allocateRegisters(uint32_t useRegister, uint32_t& maxRegister) {};
	virtual void generateCode(ExpressionDataWriter& writer) = 0;

	virtual bool isConstant() const = 0;
	virtual ResultInfo getResultInfo() const = 0;
	eASTNodeType nodeType() const { return NodeType; }
	eExpType exprType() const { return ExprType; }

	const char* getOperatorAsString() const;
};


class ASTNodeNonLeaf : public ASTNode
{
protected:
	ASTNode *leftChild, *rightChild;
	uint32_t resultRegister;

public:
	ASTNodeNonLeaf(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNode(_nodeType)
		, leftChild(_leftChild)
		, rightChild(_rightChild)
		, resultRegister(UINT32_MAX)
	{}
	virtual ~ASTNodeNonLeaf();

	virtual bool isConstant() const override { return false; }
	virtual bool constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter) override;
	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual void allocateRegisters(uint32_t useRegister, uint32_t& maxRegister) override;

	virtual ResultInfo getResultInfo() const override;

	//ASTNode *leftChild() const { return leftChild; }
	//ASTNode *rightChild() const { return rightChild; }
};


class ASTNodeConst : public ASTNode
{
public:
	ASTNodeConst(eASTNodeType t)
		: ASTNode(t)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override { return true; }
	virtual bool isConstant() const override { return true; }
	virtual void generateCode(ExpressionDataWriter& writer) override {}
};


class ASTNodeConstNumber : public ASTNodeConst
{
	float value;
	ExpressionSlotIndex constSlotIndex;

public:
	ASTNodeConstNumber(float _value)
		: ASTNodeConst(eASTNodeType::VALUE_FLOAT)
		, value(_value)
		, constSlotIndex(EXP_SLOT_INDEX_MAX)
	{
		ExprType = eExpType::NUMBER;
	}

	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual ResultInfo getResultInfo() const override;

	float getValue() const { return value; }
};


class ASTNodeConstName : public ASTNodeConst
{
	Name value;
	ExpressionSlotIndex constSlotIndex;

public:
	ASTNodeConstName(Name _value)
		: ASTNodeConst(eASTNodeType::VALUE_NAME)
		, value(_value)
		, constSlotIndex(EXP_SLOT_INDEX_MAX)
	{
		ExprType = eExpType::NAME;
	}

	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual ResultInfo getResultInfo() const override;

	Name getValue() const { return value; }
};


class ASTNodeConstBool : public ASTNodeConst
{
	bool value;

public:
	ASTNodeConstBool(bool _value)
		: ASTNodeConst(eASTNodeType::VALUE_BOOL)
		, value(_value)
	{
		ExprType = eExpType::BOOL;
	}

	virtual ResultInfo getResultInfo() const override;

	bool getValue() const { return value; }
};



class ASTNodeLogic : public ASTNodeNonLeaf
{
public:
	ASTNodeLogic(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
	virtual void generateCode(ExpressionDataWriter& writer) override;
};


class ASTNodeComp : public ASTNodeNonLeaf
{
public:
	ASTNodeComp(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
	virtual void generateCode(ExpressionDataWriter& writer) override;
};


class ASTNodeArith : public ASTNodeNonLeaf
{
public:
	ASTNodeArith(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
	virtual void generateCode(ExpressionDataWriter& writer) override;
};


class ASTNodeID : public ASTNode
{
	const Name name;
	ExpressionSlotIndex slotIndex;

public:
	ASTNodeID(const char *_name)
		: ASTNode(eASTNodeType::IDENT)
		, name(_name)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool isConstant() const override { return false; }
	virtual void generateCode(ExpressionDataWriter& writer) override {}
	virtual ResultInfo getResultInfo() const override;

	Name getName() const { return name; }
};


/*
 * ASTNode
 *
 */

bool ASTNode::constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	return constFoldThisNode(parentPointerToThis, reporter);
}

const char* ASTNode::getOperatorAsString() const
{
	switch (NodeType)
	{
	case eASTNodeType::LOGICAL_OR:		return "||";
	case eASTNodeType::LOGICAL_AND:		return "&&";
	case eASTNodeType::LOGICAL_NOT:		return "!";
	case eASTNodeType::COMP_EQ:			return "==";
	case eASTNodeType::COMP_NEQ:		return "!=";
	case eASTNodeType::COMP_LT:			return "<";
	case eASTNodeType::COMP_LTEQ:		return "<=";
	case eASTNodeType::COMP_GT:			return ">";
	case eASTNodeType::COMP_GTEQ:		return ">=";
	case eASTNodeType::ARITH_ADD:		return "+";
	case eASTNodeType::ARITH_SUB:		return "-";
	case eASTNodeType::ARITH_MUL:		return "*";
	case eASTNodeType::ARITH_DIV:		return "/";
	case eASTNodeType::ARITH_MOD:		return "%";

	default:
		assert(false);
		return "";
	}
}



/*
 * ASTNodeNonLeaf
 *
 */

ASTNodeNonLeaf::~ASTNodeNonLeaf()
{
	if (leftChild)
	{
		delete leftChild;
	}

	if (rightChild)
	{
		delete rightChild;
	}
}

bool ASTNodeNonLeaf::constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	ASTNode *tempLeftChild(leftChild);
	bool leftChildResult = leftChild->constFold(&leftChild, reporter);
	if (tempLeftChild != leftChild)
	{
		freeNode(tempLeftChild);
	}
	if (!leftChildResult) return false;

	if (rightChild)
	{
		ASTNode *tempRightChild(rightChild);
		bool rightChildResult = rightChild->constFold(&rightChild, reporter);
		if (tempRightChild != rightChild)
		{
			freeNode(tempRightChild);
		}
		if (!rightChildResult) return false;
	}

	return constFoldThisNode(parentPointerToThis, reporter);
}

void ASTNodeNonLeaf::gatherConsts(ExpressionDataWriter& writer)
{
	leftChild->gatherConsts(writer);
	if (rightChild)
	{
		rightChild->gatherConsts(writer);
	}
}

void ASTNodeNonLeaf::allocateRegisters(uint32_t useRegister, uint32_t& maxRegister)
{
	resultRegister = useRegister;
	if (useRegister > maxRegister)
	{
		maxRegister = useRegister;
	}

	leftChild->allocateRegisters(useRegister, maxRegister);
	if (rightChild)
	{
		rightChild->allocateRegisters(useRegister + 1, maxRegister);
	}
}

ResultInfo ASTNodeNonLeaf::getResultInfo() const
{
	return ResultInfo(eResultSource::Register, resultRegister);
}



/*
 * ASTNodeLogic
 *
 */

bool ASTNodeLogic::typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter)
{
	if (!leftChild->typeCheck(varLayout, reporter)) return false;
	if (rightChild && !rightChild->typeCheck(varLayout, reporter)) return false;

	if (leftChild->exprType() != eExpType::BOOL ||
		(rightChild && rightChild->exprType() != eExpType::BOOL))
	{
		std::ostringstream msg;
		if (nodeType() == eASTNodeType::LOGICAL_NOT)
		{
			msg << "Right side of " << getOperatorAsString() << " must be boolean";
		}
		else
		{
			msg << "Both sides of " << getOperatorAsString() << " must be boolean";	
		}
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::LogicTypeError, msg.str());

		return false;
	}

	ExprType = eExpType::BOOL;
	
	return true;
}

bool ASTNodeLogic::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (nodeType() == eASTNodeType::LOGICAL_NOT)
	{
		if (leftChild->isConstant())
		{
			assert(leftChild->exprType() == eExpType::BOOL);
			const bool leftVal = static_cast<ASTNodeConstBool*>(leftChild)->getValue();

			*parentPointerToThis = createConstNode(!leftVal);
		}
	}
	else if (nodeType() == eASTNodeType::LOGICAL_AND)
	{
		if (leftChild->isConstant() || rightChild->isConstant())
		{
			assert(leftChild->exprType() == eExpType::BOOL);
			assert(rightChild && rightChild->exprType() == eExpType::BOOL);

			const bool leftVal = leftChild->isConstant() ? static_cast<ASTNodeConstBool*>(leftChild)->getValue() : true;
			const bool rightVal = rightChild->isConstant() ? static_cast<ASTNodeConstBool*>(rightChild)->getValue() : true;

			if (!(leftVal && rightVal))
			{
				*parentPointerToThis = createConstNode(false);
			}
			else if (leftChild->isConstant())
			{
				*parentPointerToThis = rightChild;
				rightChild = nullptr;
			}
			else
			{
				*parentPointerToThis = leftChild;
				leftChild = nullptr;
			}
		}
	}
	else if (nodeType() == eASTNodeType::LOGICAL_OR)
	{
		if (leftChild->isConstant() || rightChild->isConstant())
		{
			assert(leftChild->exprType() == eExpType::BOOL);
			assert(rightChild && rightChild->exprType() == eExpType::BOOL);

			const bool leftVal = leftChild->isConstant() ? static_cast<ASTNodeConstBool*>(leftChild)->getValue() : false;
			const bool rightVal = rightChild->isConstant() ? static_cast<ASTNodeConstBool*>(rightChild)->getValue() : false;

			if (leftVal || rightVal)
			{
				*parentPointerToThis = createConstNode(true);
			}
			else if (leftChild->isConstant())
			{
				*parentPointerToThis = rightChild;
				rightChild = nullptr;
			}
			else
			{
				*parentPointerToThis = leftChild;
				leftChild = nullptr;
			}
		}
	}
	else
	{
		assert(false);
		return false;
	}

	return true;
}

void ASTNodeLogic::generateCode(ExpressionDataWriter& writer)
{
	leftChild->generateCode(writer);
	if (rightChild)
	{
		rightChild->generateCode(writer);
	}

	ResultInfo leftRI = leftChild->getResultInfo();
	ResultInfo rightRI = rightChild ? rightChild->getResultInfo() : leftRI;

	eSimpleOp simpleOp(eSimpleOp::UNINITIALISED);
	switch (nodeType())
	{
	case eASTNodeType::LOGICAL_NOT:	simpleOp = eSimpleOp::NOT; break;
	case eASTNodeType::LOGICAL_AND:	simpleOp = eSimpleOp::AND; break;
	case eASTNodeType::LOGICAL_OR:	simpleOp = eSimpleOp::OR;  break;

	default:
		assert(false);
	}

	eEncOpcode encOp = encodeOp(simpleOp, leftRI.source, rightRI.source);

	writer.emitInstr(encOp, resultRegister, leftRI.index, rightRI.index);
}


/*
 * ASTNodeComp
 *
 */

bool ASTNodeComp::typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter)
{
	if (!leftChild->typeCheck(varLayout, reporter)) return false;
	if (!rightChild->typeCheck(varLayout, reporter)) return false;

	ExprType = eExpType::BOOL;

	if (leftChild->exprType() != rightChild->exprType())
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be the same type";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ComparisonTypeError, msg.str());

		return false;
	}

	if (leftChild->exprType() == eExpType::BOOL || leftChild->exprType() == eExpType::NAME)
	{
		switch (nodeType())
		{
		case eASTNodeType::COMP_NEQ:
		case eASTNodeType::COMP_EQ:
			// these are OK
			break;

		default:
			{
				std::ostringstream msg;
				msg << "Operator " << getOperatorAsString() << " is invalid with " << getTypeAsString(leftChild->exprType()) << " operands";
				reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ComparisonTypeError, msg.str());
				return false;
			}
		}
	}

	return true;
}

bool ASTNodeComp::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (leftChild->isConstant() && rightChild->isConstant())
	{
		assert(leftChild->exprType() == rightChild->exprType());

		switch (leftChild->exprType())
		{
		case eExpType::BOOL:
			{
				const bool leftVal = static_cast<ASTNodeConstBool*>(leftChild)->getValue();
				const bool rightVal = static_cast<ASTNodeConstBool*>(rightChild)->getValue();
				bool newVal(false);

				switch (nodeType())
				{
				case eASTNodeType::COMP_EQ: newVal = leftVal == rightVal; break;
				case eASTNodeType::COMP_NEQ: newVal = leftVal != rightVal; break;

				default:
					assert(false);
					return false;
				}

				*parentPointerToThis = createConstNode(newVal);
			}
			break;

		case eExpType::NAME:
			{
				const Name leftVal = static_cast<ASTNodeConstName*>(leftChild)->getValue();
				const Name rightVal = static_cast<ASTNodeConstName*>(rightChild)->getValue();
				bool newVal(false);

				switch (nodeType())
				{
				case eASTNodeType::COMP_EQ: newVal = leftVal == rightVal; break;
				case eASTNodeType::COMP_NEQ: newVal = leftVal != rightVal; break;

				default:
					assert(false);
					return false;
				}

				*parentPointerToThis = createConstNode(newVal);
			}
			break;

		case eExpType::NUMBER:
			{
				const float leftVal = static_cast<ASTNodeConstNumber*>(leftChild)->getValue();
				const float rightVal = static_cast<ASTNodeConstNumber*>(rightChild)->getValue();
				bool newVal(false);

				switch (nodeType())
				{
				case eASTNodeType::COMP_EQ: newVal = leftVal == rightVal; break;
				case eASTNodeType::COMP_NEQ: newVal = leftVal != rightVal; break;
				case eASTNodeType::COMP_GT: newVal = leftVal > rightVal; break;
				case eASTNodeType::COMP_GTEQ: newVal = leftVal >= rightVal; break;
				case eASTNodeType::COMP_LT: newVal = leftVal < rightVal; break;
				case eASTNodeType::COMP_LTEQ: newVal = leftVal <= rightVal; break;

				default:
					assert(false);
					return false;
				}

				*parentPointerToThis = createConstNode(newVal);
			}

			break;

		default:
			assert(false);
			return false;
		}
	}

	return true;
}

void ASTNodeComp::generateCode(ExpressionDataWriter& writer)
{
	leftChild->generateCode(writer);
	if (rightChild)
	{
		rightChild->generateCode(writer);
	}

	ResultInfo leftRI = leftChild->getResultInfo();
	ResultInfo rightRI = rightChild ? rightChild->getResultInfo() : leftRI;

	eSimpleOp simpleOp(eSimpleOp::UNINITIALISED);

	if (leftChild->exprType() == eExpType::NUMBER)
	{
		eASTNodeType nt(nodeType());

		if ((leftRI.source == eResultSource::Register && rightRI.source != eResultSource::Register) ||
			(leftRI.source == eResultSource::Constant && rightRI.source == eResultSource::Variable))
		{
			std::swap(leftRI, rightRI);
		
			switch (nt)
			{
			case eASTNodeType::COMP_LT:		nt = eASTNodeType::COMP_GT;   break;
			case eASTNodeType::COMP_LTEQ:	nt = eASTNodeType::COMP_GTEQ; break;
			case eASTNodeType::COMP_GT:		nt = eASTNodeType::COMP_LT;   break;
			case eASTNodeType::COMP_GTEQ:	nt = eASTNodeType::COMP_LTEQ; break;
			}
		}

		switch (nt)
		{
		case eASTNodeType::COMP_EQ:		simpleOp = eSimpleOp::NUM_EQ;   break;
		case eASTNodeType::COMP_NEQ:	simpleOp = eSimpleOp::NUM_NEQ;  break;
		case eASTNodeType::COMP_LT:		simpleOp = eSimpleOp::NUM_LT;   break;
		case eASTNodeType::COMP_LTEQ:	simpleOp = eSimpleOp::NUM_LTEQ; break;
		case eASTNodeType::COMP_GT:		simpleOp = eSimpleOp::NUM_GT;   break;
		case eASTNodeType::COMP_GTEQ:	simpleOp = eSimpleOp::NUM_GTEQ; break;

		default:
			assert(false);
		}
	}
	else if (leftChild->exprType() == eExpType::NAME)
	{
		if (rightRI.source == eResultSource::Constant)
		{
			std::swap(leftRI, rightRI);
		}

		switch (nodeType())
		{
		case eASTNodeType::COMP_EQ:		simpleOp = eSimpleOp::NAME_EQ;  break;
		case eASTNodeType::COMP_NEQ:	simpleOp = eSimpleOp::NAME_NEQ; break;
		
		default:
			assert(false);
		}
	}
	else if (leftChild->exprType() == eExpType::BOOL)
	{
		switch (nodeType())
		{
		case eASTNodeType::COMP_EQ:		simpleOp = eSimpleOp::BOOL_EQ; break;
		case eASTNodeType::COMP_NEQ:	simpleOp = eSimpleOp::XOR;     break;
		
		default:
			assert(false);
		}
	}
	else
	{
		assert(false);
	}

	eEncOpcode encOp = encodeOp(simpleOp, leftRI.source, rightRI.source);

	writer.emitInstr(encOp, resultRegister, leftRI.index, rightRI.index);
}


/*
 * ASTNodeArith
 *
 */

bool ASTNodeArith::typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter)
{
	if (!leftChild->typeCheck(varLayout, reporter)) return false;
	if (!rightChild->typeCheck(varLayout, reporter)) return false;

	if (leftChild->exprType() != eExpType::NUMBER ||
		rightChild->exprType() != eExpType::NUMBER)
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be numeric";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ArithmeticTypeError, msg.str());

		return false;
	}

	ExprType = eExpType::NUMBER;
	
	return true;
}

bool ASTNodeArith::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (leftChild->isConstant() && rightChild->isConstant())
	{
		assert(leftChild->exprType() == eExpType::NUMBER);
		assert(rightChild->exprType() == eExpType::NUMBER);

		const float leftValue = reinterpret_cast<ASTNodeConstNumber*>(leftChild)->getValue();
		const float rightValue = reinterpret_cast<ASTNodeConstNumber*>(rightChild)->getValue();
		float result(0.f);

		switch (nodeType())
		{
		case eASTNodeType::ARITH_ADD: result = leftValue + rightValue; break;
		case eASTNodeType::ARITH_SUB: result = leftValue - rightValue; break;
		case eASTNodeType::ARITH_MUL: result = leftValue * rightValue; break;
		case eASTNodeType::ARITH_DIV:
			if (rightValue == 0.f)
			{
				std::ostringstream msg;
				msg << "Divide by zero detected: " << leftValue << "/" << rightValue;
				reporter.addError(eErrorCategory::Math, eErrorCode::DivideByZero, msg.str());
				return false;
			}
			result = leftValue / rightValue; 
			break;

		case eASTNodeType::ARITH_MOD: result = remainderf(leftValue, rightValue); break;

		default:
			assert(false);
		}

		*parentPointerToThis = createConstNode(result);
	}

	return true;
}

void ASTNodeArith::generateCode(ExpressionDataWriter& writer)
{
	leftChild->generateCode(writer);
	if (rightChild)
	{
		rightChild->generateCode(writer);
	}

	ResultInfo leftRI = leftChild->getResultInfo();
	ResultInfo rightRI = rightChild ? rightChild->getResultInfo() : leftRI;

	// swap left and right where necessary to account for reduced redundant instruction encodings
	if (nodeType() == eASTNodeType::ARITH_ADD || nodeType() == eASTNodeType::ARITH_MUL)
	{
		if ((leftRI.source == eResultSource::Register && rightRI.source != eResultSource::Register) ||
			(leftRI.source == eResultSource::Variable && rightRI.source == eResultSource::Constant))
		{
			std::swap(leftRI, rightRI);
		}
	}

	eSimpleOp simpleOp(eSimpleOp::UNINITIALISED);
	switch (nodeType())
	{
	case eASTNodeType::ARITH_ADD:	simpleOp = eSimpleOp::ADD; break;
	case eASTNodeType::ARITH_SUB:	simpleOp = eSimpleOp::SUB; break;
	case eASTNodeType::ARITH_MUL:	simpleOp = eSimpleOp::MUL; break;
	case eASTNodeType::ARITH_DIV:	simpleOp = eSimpleOp::DIV; break;
	case eASTNodeType::ARITH_MOD:	simpleOp = eSimpleOp::MOD; break;

	default:
		assert(false);
	}

	eEncOpcode encOp = encodeOp(simpleOp, leftRI.source, rightRI.source);

	writer.emitInstr(encOp, resultRegister, leftRI.index, rightRI.index);
}


/*
 * ASTNodeConst
 *
 */

void ASTNodeConstNumber::gatherConsts(ExpressionDataWriter& writer)
{
	constSlotIndex = writer.addNumericConst(getValue());
}

ResultInfo ASTNodeConstNumber::getResultInfo() const
{
	return ResultInfo(eResultSource::Constant, constSlotIndex);
}

void ASTNodeConstName::gatherConsts(ExpressionDataWriter& writer)
{
	constSlotIndex = writer.addNameConst(getValue());
}

ResultInfo ASTNodeConstName::getResultInfo() const
{
	return ResultInfo(eResultSource::Constant, constSlotIndex);
}

ResultInfo ASTNodeConstBool::getResultInfo() const
{
	return ResultInfo(eResultSource::Constant, 0);
}


/*
 * ASTNodeID
 *
 */

bool ASTNodeID::typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter)
{
	if (!varLayout.variableExists(name))
	{
		std::ostringstream msg;
		msg << "Variable '" << name.c_str() << "' does not exist";
		reporter.addError(eErrorCategory::Identifier, eErrorCode::IdentifierNotFound, msg.str());
	
		return false;
	}

	slotIndex = varLayout.getIndex(name);
	ExprType = varLayout.getType(name);

	return true;
}

ResultInfo ASTNodeID::getResultInfo() const
{
	return ResultInfo(eResultSource::Variable, slotIndex);
}


/*
 * Node creation functions
 */

ASTNode* createNode(eASTNodeType _nodeType, ASTNode* _leftChild, ASTNode* _rightChild)
{
	ASTNode* retval(nullptr);

	switch (_nodeType)
	{
	case eASTNodeType::LOGICAL_OR:	retval = new ASTNodeLogic(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::LOGICAL_AND:	retval = new ASTNodeLogic(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::LOGICAL_NOT:	retval = new ASTNodeLogic(_nodeType, _leftChild, _rightChild); break;

	case eASTNodeType::COMP_EQ:		retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::COMP_NEQ:	retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::COMP_LT:		retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::COMP_LTEQ:	retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::COMP_GT:		retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::COMP_GTEQ:	retval = new ASTNodeComp(_nodeType, _leftChild, _rightChild); break;

	case eASTNodeType::ARITH_ADD:	retval = new ASTNodeArith(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::ARITH_SUB:	retval = new ASTNodeArith(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::ARITH_MUL:	retval = new ASTNodeArith(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::ARITH_DIV:	retval = new ASTNodeArith(_nodeType, _leftChild, _rightChild); break;
	case eASTNodeType::ARITH_MOD:	retval = new ASTNodeArith(_nodeType, _leftChild, _rightChild); break;

	default:
		assert(0);
	}

	return retval;
}

ASTNode *createConstNode(float _value)
{
	return new ASTNodeConstNumber(_value);
}

ASTNode *createConstNode(bool _value)
{
	return new ASTNodeConstBool(_value);
}

ASTNode *createConstNode(const char *_value)
{
	Name nameValue(_value);
	return new ASTNodeConstName(nameValue);
}

ASTNode *createIDNode(const char *_id)
{
	return new ASTNodeID(_id);
}

void freeNode(ASTNode *node)
{
	assert(node);
	delete node;
}


/*
 * VariableLayout
 *
 */

ExpressionSlotIndex VariableLayout::addVariable(Name name, eExpType type)
{
	if (variableExists(name))
	{
		assert(getType(name) == type);
		return getIndex(name);
	}

	ExpressionSlotIndex slotIndex(0);

	if (type == eExpType::NUMBER)
	{
		slotIndex = numberCount;
		numberCount += 1;
	}
	else if (type == eExpType::NAME)
	{
		slotIndex = nameCount;
		nameCount += 1;
	}
	else
	{
		// bool variables not allowed
		assert(false);
		return 0;
	}

	layout.emplace(name, Info(type, slotIndex));
	return slotIndex;
}


/*
 * ExpressionDataWriter
 */

ExpressionDataWriter::ExpressionDataWriter()
{
	data = new ExpressionData();
}

ExpressionDataWriter::~ExpressionDataWriter()
{
	if (data)
	{
		delete data;
	}
}

ExpressionSlotIndex ExpressionDataWriter::addNumericConst(float value)
{
	for (size_t i = 0; i < data->const_floats.size(); i++)
	{
		if (data->const_floats[i] == value)
		{
			return static_cast<ExpressionSlotIndex>(i);
		}
	}

	data->const_floats.push_back(value);
	return static_cast<ExpressionSlotIndex>(data->const_floats.size()-1);
}

ExpressionSlotIndex ExpressionDataWriter::addNameConst(Name value)
{
	for (size_t i = 0; i < data->const_names.size(); i++)
	{
		if (data->const_names[i] == value)
		{
			return static_cast<ExpressionSlotIndex>(i);
		}
	}

	data->const_names.push_back(value);
	return static_cast<ExpressionSlotIndex>(data->const_names.size()-1);
}

void ExpressionDataWriter::emitInstr(eEncOpcode opcode, ExpressionSlotIndex resultReg, ExpressionSlotIndex leftOperand, ExpressionSlotIndex rightOperand)
{
	uint32_t codeA = (static_cast<uint16_t>(opcode) << 16) | (resultReg & 0xffff);
	uint32_t codeB = (leftOperand << 16) | (rightOperand & 0xffff);

	data->byteCode.push_back(codeA);
	data->byteCode.push_back(codeB);
}

ExpressionData* ExpressionDataWriter::getData()
{
	ExpressionData* tempData = data;
	data = nullptr;

	return tempData;
}


/*
 * ExpressionEvaluator
 *
 */

ExpressionEvaluator::ExpressionEvaluator(const VariablePack* _variables)
	: variables(_variables)
{}

#define GET_LEFT_REG (reg[leftOp])
#define GET_LEFT_REG_BOOL (reg[leftOp] != 0.f)
#define GET_LEFT_NUM_VAR (variables->getVariableNumber(leftOp))
#define GET_LEFT_NAME_VAR (variables->getVariableName(leftOp))
#define GET_LEFT_NUM_CONST (exprData->const_floats[leftOp])
#define GET_LEFT_NAME_CONST (exprData->const_names[leftOp])
#define GET_RIGHT_REG (reg[rightOp])
#define GET_RIGHT_REG_BOOL (reg[rightOp] != 0.f)
#define GET_RIGHT_NUM_VAR (variables->getVariableNumber(rightOp))
#define GET_RIGHT_NAME_VAR (variables->getVariableName(rightOp))
#define GET_RIGHT_NUM_CONST (exprData->const_floats[rightOp])
#define GET_RIGHT_NAME_CONST (exprData->const_names[rightOp])

void ExpressionEvaluator::evaluate(const ExpressionData* exprData)
{
	assert(exprData);
	assert(variables);

	errorReport.reset();
	resultType = exprData->resultType;

	reg.resize(exprData->regCount, 0);

	const uint32_t codeLen(exprData->byteCode.size());
	assert((codeLen & 1) == 0);

	for (uint32_t IP = 0; IP < codeLen; ++IP)
	{
		const uint32_t byteCodeA = exprData->byteCode[IP];
		const uint32_t byteCodeB = exprData->byteCode[++IP];

		const eEncOpcode op = static_cast<eEncOpcode>(byteCodeA >> 16);
		const ExpressionSlotIndex leftOp = static_cast<ExpressionSlotIndex>(byteCodeB >> 16);
		const ExpressionSlotIndex rightOp = static_cast<ExpressionSlotIndex>(byteCodeB & 0xffff);

		float result;

		switch (op)
		{
		case eEncOpcode::ADD:			result = GET_LEFT_REG + GET_RIGHT_REG; break;
		case eEncOpcode::ADD_LC:		result = GET_LEFT_NUM_CONST + GET_RIGHT_REG; break;
		case eEncOpcode::ADD_LV:		result = GET_LEFT_NUM_VAR + GET_RIGHT_REG; break;
		case eEncOpcode::ADD_LV_RV:		result = GET_LEFT_NUM_VAR + GET_RIGHT_NUM_VAR; break;
		case eEncOpcode::ADD_LC_RV:		result = GET_LEFT_NUM_CONST + GET_RIGHT_NUM_VAR; break;

		case eEncOpcode::SUB:			result = GET_LEFT_REG - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_LC:		result = GET_LEFT_NUM_CONST - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_LV:		result = GET_LEFT_NUM_VAR - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_RC:		result = GET_LEFT_REG - GET_RIGHT_NUM_CONST; break;
		case eEncOpcode::SUB_RV:		result = GET_LEFT_REG - GET_RIGHT_NUM_VAR; break;
		case eEncOpcode::SUB_LC_RV:		result = GET_LEFT_NUM_CONST - GET_RIGHT_NUM_VAR; break;
		case eEncOpcode::SUB_LV_RC:		result = GET_LEFT_NUM_VAR - GET_RIGHT_NUM_CONST; break;
		case eEncOpcode::SUB_LV_RV:		result = GET_LEFT_NUM_VAR - GET_RIGHT_NUM_VAR; break;

		case eEncOpcode::MUL:			result = GET_LEFT_REG * GET_RIGHT_REG; break;
		case eEncOpcode::MUL_LC:		result = GET_LEFT_NUM_CONST * GET_RIGHT_REG; break;
		case eEncOpcode::MUL_LV:		result = GET_LEFT_NUM_VAR * GET_RIGHT_REG; break;
		case eEncOpcode::MUL_LV_RV:		result = GET_LEFT_NUM_VAR * GET_RIGHT_NUM_VAR; break;
		case eEncOpcode::MUL_LC_RV:		result = GET_LEFT_NUM_CONST * GET_RIGHT_NUM_VAR; break;

		case eEncOpcode::DIV:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_REG / right; break;
			}
		case eEncOpcode::DIV_LC:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_CONST / right; break;
			}
		case eEncOpcode::DIV_LV:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_VAR / right; break;
			}

		case eEncOpcode::DIV_RC:
			{
				const float right = GET_RIGHT_NUM_CONST;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_REG / right; break;
			}
		case eEncOpcode::DIV_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_REG / right; break;
			}

		case eEncOpcode::DIV_LC_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_CONST / right; break;
			}
		case eEncOpcode::DIV_LV_RC:
			{
				const float right = GET_RIGHT_NUM_CONST;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_VAR / right; break;
			}
		case eEncOpcode::DIV_LV_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_VAR / right; break;
			}

		case eEncOpcode::MOD:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_REG, right); break;
			}
		case eEncOpcode::MOD_LC:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_CONST, right); break;
			}
		case eEncOpcode::MOD_LV:
			{
				const float right = GET_RIGHT_REG;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_VAR, right); break;
			}
		case eEncOpcode::MOD_RC:
			{
				const float right = GET_RIGHT_NUM_CONST;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_REG, right); break;
			}
		case eEncOpcode::MOD_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_REG, right); break;
			}
		case eEncOpcode::MOD_LC_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_CONST, right); break;
			}
		case eEncOpcode::MOD_LV_RC:
			{
				const float right = GET_RIGHT_NUM_CONST;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_VAR, right); break;
			}
		case eEncOpcode::MOD_LV_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_VAR, right); break;
			}

		// Logic (Boolean)
		case eEncOpcode::AND:			result = GET_LEFT_REG_BOOL && GET_RIGHT_REG_BOOL ? 1.f : 0.f; break;
		case eEncOpcode::OR:			result = GET_LEFT_REG_BOOL || GET_RIGHT_REG_BOOL ? 1.f : 0.f; break;
		case eEncOpcode::XOR:			result = GET_LEFT_REG_BOOL ^ GET_RIGHT_REG_BOOL ? 1.f : 0.f; break;
		case eEncOpcode::NOT:			result = !GET_LEFT_REG_BOOL ? 1.f : 0.f; break;

		// Comparison (Names)
		case eEncOpcode::NAME_EQ_LC_RV:  result = GET_LEFT_NAME_CONST == GET_RIGHT_NAME_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NAME_EQ_LV_RV:  result = GET_LEFT_NAME_VAR   == GET_RIGHT_NAME_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NAME_NEQ_LC_RV: result = GET_LEFT_NAME_CONST != GET_RIGHT_NAME_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NAME_NEQ_LV_RV: result = GET_LEFT_NAME_VAR   != GET_RIGHT_NAME_VAR ? 1.f : 0.f; break;

		// Comparison (Boolean) [NEQ is handled by XOR]
		case eEncOpcode::BOOL_EQ:        result = GET_LEFT_REG_BOOL == GET_RIGHT_REG_BOOL ? 1.f : 0.f; break;
		
		// Comparison (Numeric)
		case eEncOpcode::NUM_EQ:		result = GET_LEFT_REG       == GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_EQ_LC:		result = GET_LEFT_NUM_CONST == GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_EQ_LV:		result = GET_LEFT_NUM_VAR   == GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_EQ_LV_RV:  result = GET_LEFT_NUM_VAR   == GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_EQ_LV_RC:	result = GET_LEFT_NUM_VAR   == GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		case eEncOpcode::NUM_NEQ:		result = GET_LEFT_REG       != GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_NEQ_LC:	result = GET_LEFT_NUM_CONST != GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_NEQ_LV:	result = GET_LEFT_NUM_VAR   != GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_NEQ_LV_RV: result = GET_LEFT_NUM_VAR   != GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_NEQ_LV_RC:	result = GET_LEFT_NUM_VAR   != GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		case eEncOpcode::NUM_LT:		result = GET_LEFT_REG       < GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LT_LC:		result = GET_LEFT_NUM_CONST < GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LT_LV:		result = GET_LEFT_NUM_VAR   < GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LT_LV_RV:	result = GET_LEFT_NUM_VAR   < GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LT_LV_RC:	result = GET_LEFT_NUM_VAR   < GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		case eEncOpcode::NUM_GT:		result = GET_LEFT_REG       > GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GT_LC:		result = GET_LEFT_NUM_CONST > GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GT_LV:		result = GET_LEFT_NUM_VAR   > GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GT_LV_RV:	result = GET_LEFT_NUM_VAR   > GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GT_LV_RC:	result = GET_LEFT_NUM_VAR   > GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		case eEncOpcode::NUM_LTEQ:			result = GET_LEFT_REG       <= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LTEQ_LC:		result = GET_LEFT_NUM_CONST <= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LTEQ_LV:		result = GET_LEFT_NUM_VAR   <= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LTEQ_LV_RV:	result = GET_LEFT_NUM_VAR   <= GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_LTEQ_LV_RC:	result = GET_LEFT_NUM_VAR   <= GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		case eEncOpcode::NUM_GTEQ:			result = GET_LEFT_REG       >= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GTEQ_LC:		result = GET_LEFT_NUM_CONST >= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GTEQ_LV:		result = GET_LEFT_NUM_VAR   >= GET_RIGHT_REG ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GTEQ_LV_RV:	result = GET_LEFT_NUM_VAR   >= GET_RIGHT_NUM_VAR ? 1.f : 0.f; break;
		case eEncOpcode::NUM_GTEQ_LV_RC:	result = GET_LEFT_NUM_VAR   >= GET_RIGHT_NUM_CONST ? 1.f : 0.f; break;

		// value operations (for const expressions)
		case eEncOpcode::NUM_VAL_LC:		result = GET_LEFT_NUM_CONST; break;
		case eEncOpcode::BOOL_VAL_LC:		result = leftOp > 0 ? 1.f : 0.f; break;

		default:
			assert(false);
			return;
		}	

		const ExpressionSlotIndex outReg = static_cast<ExpressionSlotIndex>(byteCodeA & 0xffff);
		reg[outReg] = result;
	}
}

void ExpressionEvaluator::logDivideByZeroError()
{
	errorReport.addError(eErrorCategory::Math, eErrorCode::DivideByZero, "Divide by zero error");
}

void ExpressionEvaluator::reset()
{
	errorReport.reset();
}


/*
 * ExpressionCompiler
 *
 */

#include "GeneratedFiles/FormulaParser.h"
#include "GeneratedFiles/FormulaLexer.h"

int yyparse(ASTNode **expression, yyscan_t scanner);


ExpressionCompiler::ExpressionCompiler(const VariableLayout* _layout)
	: layout(_layout)
{
	assert(layout != nullptr);
}

ExpressionData* ExpressionCompiler::compile(const char* expressionText)
{
	// parse the expression
	ASTNode *expression(nullptr);
    yyscan_t scanner;
    YY_BUFFER_STATE state;
 
    if (yylex_init(&scanner)) 
	{
        // couldn't initialize
		errorReport.addError(eErrorCategory::Internal, eErrorCode::InternalError, "Couldn't initialise parser");

        return nullptr;
    }
 
    state = yy_scan_string(expressionText, scanner);
 
    if (yyparse(&expression, scanner))
	{
        // error parsing
		errorReport.addError(eErrorCategory::Syntax, eErrorCode::SyntaxError, "Syntax error");
    
		yy_delete_buffer(state, scanner);
		yylex_destroy(scanner);

        return nullptr;
    }
 
    yy_delete_buffer(state, scanner);
    yylex_destroy(scanner);

	assert(expression != nullptr);

	// perform AST passes
	if (!expression->typeCheck(*layout, errorReport) ||
		!expression->constFold(&expression, errorReport))
	{
		freeNode(expression);
		return nullptr;
	}

	ExpressionDataWriter expWriter;

	expression->gatherConsts(expWriter);
	uint32_t maxRegister(0);
	expression->allocateRegisters(0, maxRegister);

	// generate code
	if (expression->exprType() == eExpType::NAME)
	{
		errorReport.addError(eErrorCategory::Const, eErrorCode::ConstNameExpression, "Expressions that evalute to a Name type are not supported");
		freeNode(expression);
		return nullptr;
	}
	else if (expression->isConstant())
	{
		if (expression->exprType() == eExpType::BOOL)
		{
			bool val = static_cast<ASTNodeConstBool*>(expression)->getValue();
			// we don't have a separate boolean consts array (why bother when there are only two possible values?) so encode as the slot number instead
			expWriter.emitInstr(encodeOp(eSimpleOp::BOOL_VAL, eResultSource::Constant, eResultSource::Constant), 0, val ? 1 : 0 , 0);
		}
		else if (expression->exprType() == eExpType::NUMBER)
		{
			ASTNodeConstNumber *numNode = static_cast<ASTNodeConstNumber*>(expression);
			assert(numNode->getResultInfo().source == eResultSource::Constant);
			expWriter.emitInstr(encodeOp(eSimpleOp::NUM_VAL, eResultSource::Constant, eResultSource::Constant), 0, numNode->getResultInfo().index, 0);
		}
		else
		{
			assert(false);
			return nullptr;
		}
	}
	else
	{
		expression->generateCode(expWriter);
	}

	// get generated program data and add remaining params
	ExpressionData *expData = expWriter.getData();
	assert(expData != nullptr);

	expData->regCount = maxRegister + 1;
	expData->resultType = expression->exprType();

	return expData;
}
