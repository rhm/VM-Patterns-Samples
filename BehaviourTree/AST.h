/*
 * AST Types and functions
 *
 * Separated out into their own header for the flex/bison code to include
 */

#pragma once


class ASTNode;

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

	IDENT,

	NODE_TYPE_MAX
};


/*
 * AST Node creation functions
 *
 */

ASTNode *createNode(eASTNodeType _nodeType, ASTNode* _leftChild, ASTNode* _rightChild);
ASTNode *createConstNode(float _value);
ASTNode *createConstNode(bool _value);
ASTNode *createConstNode(const char *_value);
ASTNode *createIDNode(const char *_id);

void freeNode(ASTNode *node);

