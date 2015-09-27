/*
 * Expression.h
 * Definition of the structure used to build the syntax tree.
 */
#ifndef __EXPRESSION_H__
#define __EXPRESSION_H__



enum class eASTNodeType
{
	UNINITIALISED,

    VALUE_FLOAT,
    VALUE_NAME,
	VALUE_BOOL,

	LOGICAL_OR,
	LOGICAL_AND,
	LOGICAL_NOT,
	
	COMP_EQ,
	COMP_NEQ,
	COMP_LT,
	COMP_LTEQ,
	COMP_GT,
	COMP_GTEQ,

	ARITH_ADD,
	ARITH_SUB,
	ARITH_MUL,
	ARITH_DIV,
	ARITH_MOD,
};


class ASTNode
{
	eASTNodeType m_NodeType;

protected:
	ASTNode(eASTNodeType _nodeType)
		: m_NodeType(_nodeType)
	{}

public:
	virtual ~ASTNode() {};

	eASTNodeType nodeType() const { return m_NodeType; }
};

class ASTNodeNonLeaf : public ASTNode
{
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
	union
	{
		float f_value;
		const char* n_value;
		bool b_value;
	} m_value;

public:
	ASTNodeConst(float _value)
		: ASTNode(eASTNodeType::VALUE_FLOAT)
	{
		m_value.f_value = _value;
	}

	ASTNodeConst(const char *_value)
		: ASTNode(eASTNodeType::VALUE_NAME)
	{
		m_value.n_value = _value;
	}

	ASTNodeConst(bool _value)
		: ASTNode(eASTNodeType::VALUE_BOOL)
	{
		m_value.b_value = _value;
	}


	float floatValue() const;
	const char* nameValue() const;
	bool boolValue() const;
};

class ASTNodeLogic : public ASTNodeNonLeaf
{
public:
	ASTNodeLogic(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}
};

class ASTNodeComp : public ASTNodeNonLeaf
{
public:
	ASTNodeComp(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}
};

class ASTNodeArith : public ASTNodeNonLeaf
{
public:
	ASTNodeArith(eASTNodeType _nodeType, ASTNode *_leftChild, ASTNode *_rightChild)
		: ASTNodeNonLeaf(_nodeType, _leftChild, _rightChild)
	{}
};


ASTNode *createNode(eASTNodeType _nodeType, ASTNode* _leftChild, ASTNode* _rightChild);


#endif // __EXPRESSION_H__
