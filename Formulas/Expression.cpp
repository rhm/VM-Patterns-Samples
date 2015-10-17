/*
 * Expression.c
 * Implementation of functions used to build the syntax tree.
 */

#include "stdafx.h"

#include <sstream>
#include <stdlib.h>

#include "Expression.h"
#include "Name.h"





/*
 * ExpressionErrorReporter
 *
 */

enum class eErrorCategory
{
	Syntax,
	TypeCheck,
	Identifier,
};

enum class eErrorCode
{
	SyntaxError,
	IdentifierNotFound,
	IdentifierType,
	ArithmeticTypeError,
	ComparisonTypeError,
	LogicTypeError,
};

class ExpressionErrorReporter
{
public:
	struct Info
	{
		eErrorCategory category;
		eErrorCode code;
		std::string message;
	};

private:
	std::vector<Info> m_errors;

public:
	ExpressionErrorReporter() {}
	~ExpressionErrorReporter() {}

	void addError(eErrorCategory _category, eErrorCode _code, const std::string& _message)
	{
		m_errors.emplace_back(Info{ _category, _code, _message });
	}

	uint32_t errorCount() const { return m_errors.size(); }

	const Info& error(uint32_t errorIndex) const 
	{ 
		assert(errorIndex >= 0 && errorIndex < m_errors.size());

		return m_errors[errorIndex]; 
	}
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

public:
	virtual ~ASTNode() {};

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) = 0;

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
};


class ASTNodeComp : public ASTNodeNonLeaf
{
public:
	ASTNodeComp(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
};


class ASTNodeArith : public ASTNodeNonLeaf
{
public:
	ASTNodeArith(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}

	virtual bool typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter) override;
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

	Name name() const { return m_name; }
};


/*
 * ASTNode
 *
 */

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


/*
 * ASTNodeLogic
 *
 */

bool ASTNodeLogic::typeCheck(const VariableLayout& varLayout, ExpressionErrorReporter& reporter)
{
	if (!m_leftChild->typeCheck(varLayout, reporter)) return false;
	if (!m_rightChild->typeCheck(varLayout, reporter)) return false;

	if (m_leftChild->exprType() != eExpType::BOOL ||
		m_rightChild->exprType() != eExpType::BOOL)
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be boolean";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::LogicTypeError, msg.str());

		return false;
	}

	m_ExprType = eExpType::BOOL;
	
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

	if (m_leftChild->exprType() == m_rightChild->exprType())
	{
		std::ostringstream msg;
		msg << "Both sides of " << getOperatorAsString() << " must be the same type";
		reporter.addError(eErrorCategory::TypeCheck, eErrorCode::ComparisonTypeError, msg.str());

		return false;
	}
	
	m_ExprType = eExpType::BOOL;

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

