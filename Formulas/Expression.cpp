/*
 * Expression.c
 * Implementation of functions used to build the syntax tree.
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
	Register
};

struct ResultInfo
{
	eResultSource source;
	int position;
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
	virtual bool isConstant() const = 0;
	virtual bool constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);

	//virtual ResultInfo getResultInfo() const = 0;
	eASTNodeType nodeType() const { return m_NodeType; }
	eExpType exprType() const { return m_ExprType; }

	const char* getOperatorAsString() const;
};


class ASTNodeNonLeaf : public ASTNode
{
protected:
	ASTNode *m_leftChild, *m_rightChild;

public:
	ASTNodeNonLeaf(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNode(_nodeType)
		, m_leftChild(_leftChild)
		, m_rightChild(_rightChild)
	{}
	virtual ~ASTNodeNonLeaf();

	virtual bool isConstant() const override { return false; }
	virtual bool constFold(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter) override;

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
};


class ASTNodeConstNumber : public ASTNodeConst
{
	float m_value;

public:
	ASTNodeConstNumber(float _value)
		: ASTNodeConst(eASTNodeType::VALUE_FLOAT)
		, m_value(_value)
	{
		m_ExprType = eExpType::NUMBER;
	}

	float value() const { return m_value; }
};


class ASTNodeConstName : public ASTNodeConst
{
	Name m_value;

public:
	ASTNodeConstName(Name _value)
		: ASTNodeConst(eASTNodeType::VALUE_NAME)
		, m_value(_value)
	{
		m_ExprType = eExpType::NAME;
	}

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
};


class ASTNodeComp : public ASTNodeNonLeaf
{
public:
	ASTNodeComp(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
};


class ASTNodeArith : public ASTNodeNonLeaf
{
public:
	ASTNodeArith(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool constFoldThisNode(ASTNode **parentPointerToThis, ExpressionErrorReporter& reporter);
};


class ASTNodeID : public ASTNode
{
	const Name m_name;

public:
	ASTNodeID(const char *_name)
		: ASTNode(eASTNodeType::IDENT)
		, m_name(_name)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
	virtual bool isConstant() const override { return false; }

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
	if (m_leftChild->isConstant() && m_rightChild->isConstant())
	{
		assert(m_leftChild->exprType() == eExpType::BOOL);
		assert(m_rightChild == nullptr || m_rightChild->exprType() == eExpType::BOOL);

		const bool leftVal = reinterpret_cast<ASTNodeConstBool*>(m_leftChild)->value();
		const bool rightVal = m_rightChild != nullptr ? reinterpret_cast<ASTNodeConstBool*>(m_rightChild)->value() : false;
		bool newVal(false);

		switch (nodeType())
		{
		case eASTNodeType::LOGICAL_AND: newVal = leftVal && rightVal; break;
		case eASTNodeType::LOGICAL_OR: newVal = leftVal || rightVal; break;
		case eASTNodeType::LOGICAL_NOT: newVal = !leftVal; break;

		default:
			assert(false);
			return false;
		}

		freeNode(m_leftChild);
		if (m_rightChild)
		{
			freeNode(m_rightChild);
		}
		*parentPointerToThis = createConstNode(newVal);
	}

	return true;
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


/*
 * ASTNodeConst
 *
 */



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

	m_ExprType = varLayout.getType(m_name);
	return true;
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

namespace
{
	template<typename K, typename V>
	bool KeyExists(const std::unordered_map<K, V>& map, const K& key)
	{
		auto it = map.find(key);
		return it != map.end();
	}
}

bool VariableLayout::variableExists(const Name& variableName) const
{
	return KeyExists(m_layout, variableName);
}

eExpType VariableLayout::getType(const Name& variableName) const
{
	auto it = m_layout.find(variableName);
	if (it == m_layout.end())
	{
		return eExpType::UNINITIALISED;
	}

	return it->second.type;
}

int VariableLayout::getIndex(const Name& variableName) const
{
	auto it = m_layout.find(variableName);
	if (it == m_layout.end())
	{
		return -1;
	}

	return it->second.index;
}

