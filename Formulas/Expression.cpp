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
 * Defines
 */

#define USE_OPCODE_BIT_ENCODING 1



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
	NUM_GTEQ
};

#if USE_OPCODE_BIT_ENCODING

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

	OPCODE_MAX
};

inline eEncOpcode encodeOp(eSimpleOp simpleOp, eResultSource leftSource, eResultSource rightSource)
{
	return static_cast<eEncOpcode>(OPCODE(static_cast<uint8_t>(simpleOp), static_cast<uint8_t>(leftSource), static_cast<uint8_t>(rightSource)));
}


#else // !USE_OPCODE_BIT_ENCODING




#endif // USE_OPCODE_BIT_ENCODING

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
	eASTNodeType m_NodeType;

protected:
	eExpType m_ExprType;

	ASTNode(eASTNodeType _nodeType)
		: m_NodeType(_nodeType)
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
	eASTNodeType nodeType() const { return m_NodeType; }
	eExpType exprType() const { return m_ExprType; }

	const char* getOperatorAsString() const;
};


class ASTNodeNonLeaf : public ASTNode
{
protected:
	ASTNode *m_leftChild, *m_rightChild;
	uint32_t resultRegister;

public:
	ASTNodeNonLeaf(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNode(_nodeType)
		, m_leftChild(_leftChild)
		, m_rightChild(_rightChild)
		, resultRegister(UINT32_MAX)
	{}
	virtual ~ASTNodeNonLeaf();

	virtual bool isConstant() const override { return false; }
	virtual bool constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter) override;
	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual void allocateRegisters(uint32_t useRegister, uint32_t& maxRegister) override;

	virtual ResultInfo getResultInfo() const override;

	ASTNode *leftChild() const { return m_leftChild; }
	ASTNode *rightChild() const { return m_rightChild; }
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
	float m_value;
	ExpressionSlotIndex constSlotIndex;

public:
	ASTNodeConstNumber(float _value)
		: ASTNodeConst(eASTNodeType::VALUE_FLOAT)
		, m_value(_value)
		, constSlotIndex(EXP_SLOT_INDEX_MAX)
	{
		m_ExprType = eExpType::NUMBER;
	}

	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual ResultInfo getResultInfo() const override;

	float value() const { return m_value; }
};


class ASTNodeConstName : public ASTNodeConst
{
	Name m_value;
	ExpressionSlotIndex constSlotIndex;

public:
	ASTNodeConstName(Name _value)
		: ASTNodeConst(eASTNodeType::VALUE_NAME)
		, m_value(_value)
		, constSlotIndex(EXP_SLOT_INDEX_MAX)
	{
		m_ExprType = eExpType::NAME;
	}

	virtual void gatherConsts(ExpressionDataWriter& writer) override;
	virtual ResultInfo getResultInfo() const override;

	Name value() const { return m_value; }
};


class ASTNodeConstBool : public ASTNodeConst
{
	bool m_value;

public:
	ASTNodeConstBool(bool _value)
		: ASTNodeConst(eASTNodeType::VALUE_BOOL)
		, m_value(_value)
	{
		m_ExprType = eExpType::BOOL;
	}

	virtual ResultInfo getResultInfo() const override;

	bool value() const { return m_value; }
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
	const Name m_name;
	ExpressionSlotIndex slotIndex;

public:
	ASTNodeID(const char *_name)
		: ASTNode(eASTNodeType::IDENT)
		, m_name(_name)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool isConstant() const override { return false; }
	virtual void generateCode(ExpressionDataWriter& writer) override {}
	virtual ResultInfo getResultInfo() const override;

	Name name() const { return m_name; }
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
	switch (m_NodeType)
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
	if (m_leftChild)
	{
		delete m_leftChild;
	}

	if (m_rightChild)
	{
		delete m_rightChild;
	}
}

bool ASTNodeNonLeaf::constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	ASTNode *tempLeftChild(m_leftChild);
	bool leftChildResult = m_leftChild->constFold(&m_leftChild, reporter);
	if (tempLeftChild != m_leftChild)
	{
		freeNode(tempLeftChild);
	}
	if (!leftChildResult) return false;

	if (m_rightChild)
	{
		ASTNode *tempRightChild(m_rightChild);
		bool rightChildResult = m_rightChild->constFold(&m_rightChild, reporter);
		if (tempRightChild != m_rightChild)
		{
			freeNode(tempRightChild);
		}
		if (!rightChildResult) return false;
	}

	return constFoldThisNode(parentPointerToThis, reporter);
}

void ASTNodeNonLeaf::gatherConsts(ExpressionDataWriter& writer)
{
	m_leftChild->gatherConsts(writer);
	if (m_rightChild)
	{
		m_rightChild->gatherConsts(writer);
	}
}

void ASTNodeNonLeaf::allocateRegisters(uint32_t useRegister, uint32_t& maxRegister)
{
	resultRegister = useRegister;
	if (useRegister > maxRegister)
	{
		maxRegister = useRegister;
	}

	m_leftChild->allocateRegisters(useRegister, maxRegister);
	if (m_rightChild)
	{
		m_rightChild->allocateRegisters(useRegister + 1, maxRegister);
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
	if (!m_leftChild->typeCheck(varLayout, reporter)) return false;
	if (m_rightChild && !m_rightChild->typeCheck(varLayout, reporter)) return false;

	if (m_leftChild->exprType() != eExpType::BOOL ||
		(m_rightChild && m_rightChild->exprType() != eExpType::BOOL))
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

	m_ExprType = eExpType::BOOL;
	
	return true;
}

bool ASTNodeLogic::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (nodeType() == eASTNodeType::LOGICAL_NOT && m_leftChild->isConstant())
	{
		assert(m_leftChild->exprType() == eExpType::BOOL);
		const bool leftVal = static_cast<ASTNodeConstBool*>(m_leftChild)->value();

		*parentPointerToThis = createConstNode(!leftVal);
	}
	else if (nodeType() == eASTNodeType::LOGICAL_AND &&
			 (m_leftChild->isConstant() || m_rightChild->isConstant()))
	{
		assert(m_leftChild->exprType() == eExpType::BOOL);
		assert(m_rightChild && m_rightChild->exprType() == eExpType::BOOL);

		const bool leftVal = m_leftChild->isConstant() ? static_cast<ASTNodeConstBool*>(m_leftChild)->value() : true;
		const bool rightVal = m_rightChild->isConstant() ? static_cast<ASTNodeConstBool*>(m_rightChild)->value() : true;
		
		if (!(leftVal && rightVal))
		{
			*parentPointerToThis = createConstNode(false);
		}
		else if (m_leftChild->isConstant())
		{
			*parentPointerToThis = m_rightChild;
			m_rightChild = nullptr;
		}
		else
		{
			*parentPointerToThis = m_leftChild;
			m_leftChild = nullptr;
		}
	}
	else if (nodeType() == eASTNodeType::LOGICAL_OR &&
			 (m_leftChild->isConstant() || m_rightChild->isConstant()))
	{
		assert(m_leftChild->exprType() == eExpType::BOOL);
		assert(m_rightChild && m_rightChild->exprType() == eExpType::BOOL);

		const bool leftVal = m_leftChild->isConstant() ? static_cast<ASTNodeConstBool*>(m_leftChild)->value() : false;
		const bool rightVal = m_rightChild->isConstant() ? static_cast<ASTNodeConstBool*>(m_rightChild)->value() : false;
		
		if (leftVal || rightVal)
		{
			*parentPointerToThis = createConstNode(true);
		}
		else if (m_leftChild->isConstant())
		{
			*parentPointerToThis = m_rightChild;
			m_rightChild = nullptr;
		}
		else
		{
			*parentPointerToThis = m_leftChild;
			m_leftChild = nullptr;
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
	ResultInfo leftRI = m_leftChild->getResultInfo();
	ResultInfo rightRI = m_rightChild ? m_rightChild->getResultInfo() : leftRI;

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
	if (!m_leftChild->typeCheck(varLayout, reporter)) return false;
	if (!m_rightChild->typeCheck(varLayout, reporter)) return false;

	m_ExprType = eExpType::BOOL;

	if (m_leftChild->exprType() == m_rightChild->exprType())
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be the same type";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ComparisonTypeError, msg.str());

		return false;
	}

	if (m_leftChild->exprType() == eExpType::BOOL || m_leftChild->exprType() == eExpType::NAME)
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
				msg << "Operator " << getOperatorAsString() << " is invalid with " << getTypeAsString(m_leftChild->exprType()) << " operands";
				reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ComparisonTypeError, msg.str());
				return false;
			}
		}
	}

	return true;
}

bool ASTNodeComp::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (m_leftChild->isConstant() && m_rightChild->isConstant())
	{
		switch (exprType())
		{
		case eExpType::BOOL:
			{
				assert(m_leftChild->exprType() == eExpType::BOOL);
				assert(m_rightChild->exprType() == eExpType::BOOL);
				
				const bool leftVal = reinterpret_cast<ASTNodeConstBool*>(m_leftChild)->value();
				const bool rightVal = reinterpret_cast<ASTNodeConstBool*>(m_rightChild)->value();
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
				assert(m_leftChild->exprType() == eExpType::NAME);
				assert(m_rightChild->exprType() == eExpType::NAME);
				
				const Name leftVal = reinterpret_cast<ASTNodeConstName*>(m_leftChild)->value();
				const Name rightVal = reinterpret_cast<ASTNodeConstName*>(m_rightChild)->value();
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
				assert(m_leftChild->exprType() == eExpType::NUMBER);
				assert(m_rightChild->exprType() == eExpType::NUMBER);
				
				const float leftVal = reinterpret_cast<ASTNodeConstNumber*>(m_leftChild)->value();
				const float rightVal = reinterpret_cast<ASTNodeConstNumber*>(m_rightChild)->value();
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
	ResultInfo leftRI = m_leftChild->getResultInfo();
	ResultInfo rightRI = m_rightChild ? m_rightChild->getResultInfo() : leftRI;

	eSimpleOp simpleOp(eSimpleOp::UNINITIALISED);

	if (exprType() == eExpType::NUMBER)
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
	else if (exprType() == eExpType::NAME)
	{
		switch (nodeType())
		{
		case eASTNodeType::COMP_EQ:		simpleOp = eSimpleOp::NAME_EQ;  break;
		case eASTNodeType::COMP_NEQ:	simpleOp = eSimpleOp::NAME_NEQ; break;
		
		default:
			assert(false);
		}
	}
	else if (exprType() == eExpType::BOOL)
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
	if (!m_leftChild->typeCheck(varLayout, reporter)) return false;
	if (!m_rightChild->typeCheck(varLayout, reporter)) return false;

	if (m_leftChild->exprType() != eExpType::NUMBER ||
		m_rightChild->exprType() != eExpType::NUMBER)
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be numeric";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ArithmeticTypeError, msg.str());

		return false;
	}

	m_ExprType = eExpType::NUMBER;
	
	return true;
}

bool ASTNodeArith::constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter)
{
	if (m_leftChild->isConstant() && m_rightChild->isConstant())
	{
		assert(m_leftChild->exprType() == eExpType::NUMBER);
		assert(m_rightChild->exprType() == eExpType::NUMBER);

		const float leftValue = reinterpret_cast<ASTNodeConstNumber*>(m_leftChild)->value();
		const float rightValue = reinterpret_cast<ASTNodeConstNumber*>(m_rightChild)->value();
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
	ResultInfo leftRI = m_leftChild->getResultInfo();
	ResultInfo rightRI = m_rightChild ? m_rightChild->getResultInfo() : leftRI;

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
	constSlotIndex = writer.addNumericConst(value());
}

ResultInfo ASTNodeConstNumber::getResultInfo() const
{
	return ResultInfo(eResultSource::Constant, constSlotIndex);
}

void ASTNodeConstName::gatherConsts(ExpressionDataWriter& writer)
{
	constSlotIndex = writer.addNameConst(value());
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
	if (!varLayout.variableExists(m_name))
	{
		std::ostringstream msg;
		msg << "Variable '" << m_name.c_str() << "' does not exist";
		reporter.addError(eErrorCategory::Identifier, eErrorCode::IdentifierNotFound, msg.str());
	
		return false;
	}

	slotIndex = varLayout.getIndex(m_name);
	m_ExprType = varLayout.getType(m_name);

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

	m_layout.emplace(name, Info(type, slotIndex));
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
	for (size_t i = 0; i < data->m_const_floats.size(); i++)
	{
		if (data->m_const_floats[i] == value)
		{
			return static_cast<ExpressionSlotIndex>(i);
		}
	}

	data->m_const_floats.push_back(value);
	return static_cast<ExpressionSlotIndex>(data->m_const_floats.size()-1);
}

ExpressionSlotIndex ExpressionDataWriter::addNameConst(Name value)
{
	for (size_t i = 0; i < data->m_const_names.size(); i++)
	{
		if (data->m_const_names[i] == value)
		{
			return static_cast<ExpressionSlotIndex>(i);
		}
	}

	data->m_const_names.push_back(value);
	return static_cast<ExpressionSlotIndex>(data->m_const_names.size()-1);
}

void ExpressionDataWriter::emitInstr(eEncOpcode opcode, ExpressionSlotIndex resultReg, ExpressionSlotIndex leftOperand, ExpressionSlotIndex rightOperand)
{
	uint32_t codeA = (static_cast<uint16_t>(opcode) << 16) | (resultReg & 0xffff);
	uint32_t codeB = (leftOperand << 16) | (rightOperand & 0xffff);

	data->m_byteCode.push_back(codeA);
	data->m_byteCode.push_back(codeB);
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
#define GET_LEFT_NUM_CONST (exprData->m_const_floats[leftOp])
#define GET_LEFT_NAME_CONST (exprData->m_const_names[leftOp])
#define GET_RIGHT_REG (reg[rightOp])
#define GET_RIGHT_REG_BOOL (reg[rightOp] != 0.f)
#define GET_RIGHT_NUM_VAR (variables->getVariableNumber(rightOp))
#define GET_RIGHT_NAME_VAR (variables->getVariableName(rightOp))
#define GET_RIGHT_NUM_CONST (exprData->m_const_floats[rightOp])
#define GET_RIGHT_NAME_CONST (exprData->m_const_names[rightOp])

void ExpressionEvaluator::evaluate(const ExpressionData* exprData)
{
	assert(exprData);
	assert(variables);

	errorReport.reset();
	resultType = exprData->resultType;

	reg.resize(exprData->regCount, 0);

	const uint32_t codeLen(exprData->m_byteCode.size());
	assert((codeLen & 1) == 0);

	for (uint32_t IP = 0; IP < codeLen; ++IP)
	{
		const uint32_t byteCodeA = exprData->m_byteCode[IP];
		const uint32_t byteCodeB = exprData->m_byteCode[++IP];

		const eEncOpcode op = static_cast<eEncOpcode>(byteCodeA >> 16);
		const ExpressionSlotIndex leftOp = static_cast<ExpressionSlotIndex>(byteCodeB >> 16);
		const ExpressionSlotIndex rightOp = static_cast<ExpressionSlotIndex>(byteCodeB & 0xffff);

		float result;

		switch (op)
		{
		case eEncOpcode::ADD:			result = GET_LEFT_REG + GET_RIGHT_REG; break;
		case eEncOpcode::ADD_LC:		result = GET_LEFT_NUM_CONST + GET_RIGHT_REG; break;
		case eEncOpcode::ADD_LC_RV:		result = GET_LEFT_NUM_CONST + GET_RIGHT_NUM_VAR; break;

		case eEncOpcode::SUB:			result = GET_LEFT_REG - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_LC:		result = GET_LEFT_NUM_CONST - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_LV:		result = GET_LEFT_NUM_VAR - GET_RIGHT_REG; break;
		case eEncOpcode::SUB_LC_RV:		result = GET_LEFT_NUM_CONST - GET_RIGHT_NUM_VAR; break;

		case eEncOpcode::MUL:			result = GET_LEFT_REG * GET_RIGHT_REG; break;
		case eEncOpcode::MUL_LC:		result = GET_LEFT_NUM_CONST * GET_RIGHT_REG; break;
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
		case eEncOpcode::DIV_LC_RV:		result = GET_LEFT_NUM_CONST / GET_RIGHT_NUM_VAR; break;
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = GET_LEFT_NUM_CONST / right; break;
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
		case eEncOpcode::MOD_LC_RV:
			{
				const float right = GET_RIGHT_NUM_VAR;
				if (right == 0.f) { logDivideByZeroError(); return; }
				result = fmodf(GET_LEFT_NUM_CONST, right); break;
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
		return false;
	}

	ExpressionDataWriter expWriter;

	expression->gatherConsts(expWriter);
	uint32_t maxRegister(0);
	expression->allocateRegisters(0, maxRegister);
	expression->generateCode(expWriter);

	// get generated program data and add remaining params
	ExpressionData *expData = expWriter.getData();
	assert(expData != nullptr);

	expData->regCount = maxRegister + 1;
	expData->resultType = expression->exprType();

	return expData;
}
