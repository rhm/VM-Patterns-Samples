/*
 * Expression.c
 * Implementation of functions used to build the syntax tree.
 */

#include "stdafx.h"

#include "Expression.h"

#include <stdlib.h>



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

float ASTNodeConst::floatValue() const 
{ 
	assert(nodeType() == eASTNodeType::VALUE_FLOAT); 
	return m_value.f_value; 
}

const char* ASTNodeConst::nameValue() const 
{ 
	assert(nodeType() == eASTNodeType::VALUE_NAME); 
	return m_value.n_value; 
}

bool ASTNodeConst::boolValue() const 
{ 
	assert(nodeType() == eASTNodeType::VALUE_BOOL); 
	return m_value.b_value;
}


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