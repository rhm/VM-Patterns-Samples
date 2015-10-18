/*
 * Expression.h
 * Definition of the structure used to build the syntax tree.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "AST.h"
#include "Name.h"


/*
 * Expression Type
 *
 */

enum class eExpType
{
	UNINITIALISED,

	NUMBER,
	NAME,
	BOOL
};

struct Expression
{
	std::vector<uint32_t> m_byteCode;
	std::vector<float> m_floats;
	std::vector<Name> m_names;
};


class VariableLayout
{
public:
	struct Info
	{
		eExpType type;
		int index;
	};

private:
	std::unordered_map<Name, Info> m_layout;

public:
	void addVariable(Name name, eExpType type, int index);

	bool variableExists(const Name& variableName) const;
	eExpType getType(const Name& variableName) const;
	int getIndex(const Name& variableName) const;


};


/*
 * ExpressionErrorReporter
 *
 */

enum class eErrorCategory
{
	Syntax,
	TypeCheck,
	Identifier,
	Math,
};

enum class eErrorCode
{
	SyntaxError,
	IdentifierNotFound,
	IdentifierType,
	ArithmeticTypeError,
	ComparisonTypeError,
	LogicTypeError,
	DivideByZero,
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
 * Compiler
 *
 */

class ExpressionCompiler
{




};




