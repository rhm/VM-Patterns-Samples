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

typedef uint16_t ExpressionSlotIndex;
#define EXP_SLOT_INDEX_MAX UINT16_MAX


enum class eExpType
{
	UNINITIALISED,

	NUMBER,
	NAME,
	BOOL
};

struct ExpressionData
{
	eExpType resultType;
	ExpressionSlotIndex regCount;
	std::vector<uint32_t> m_byteCode;
	std::vector<float> m_const_floats;
	std::vector<Name> m_const_names;
};


class VariableLayout
{
public:
	struct Info
	{
		eExpType type;
		ExpressionSlotIndex index;

		Info(eExpType _type, ExpressionSlotIndex _index) : type(_type), index(_index) {}
	};

private:
	std::unordered_map<Name, Info> m_layout;
	ExpressionSlotIndex numberCount, nameCount;

public:
	VariableLayout();

	ExpressionSlotIndex addVariable(Name name, eExpType type);

	bool variableExists(const Name& variableName) const;
	eExpType getType(const Name& variableName) const;
	ExpressionSlotIndex getIndex(const Name& variableName) const;

	ExpressionSlotIndex getNumberCount() const { return numberCount; }
	ExpressionSlotIndex getNameCount() const { return nameCount; }
};


class VariablePack
{
	std::vector<float> floatVars;
	std::vector<Name> nameVars;
	const VariableLayout* layout;

public:
	VariablePack(const VariableLayout* _layout, Name initName, float initNumber);
	VariablePack(const VariablePack& rhs);
	
	void setVariable(Name variableName, Name value);
	void setVariable(Name variableName, float value);
	void setVariable(ExpressionSlotIndex slotIndex, Name value);
	void setVariable(ExpressionSlotIndex slotIndex, float value);

	Name getVariableName(Name variableName) const;
	float getVariableNumber(Name variableName) const;
	Name getVariableName(ExpressionSlotIndex slotIndex) const;
	float getVariableNumber(ExpressionSlotIndex slotIndex) const;
};


/*
 * ExpressionErrorReporter
 *
 */

enum class eErrorCategory
{
	Internal,
	Syntax,
	TypeCheck,
	Identifier,
	Math,
	Const,
};

enum class eErrorCode
{
	InternalError,
	SyntaxError,
	IdentifierNotFound,
	IdentifierType,
	ArithmeticTypeError,
	ComparisonTypeError,
	LogicTypeError,
	DivideByZero,
	ConstNameExpression,
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

	void reset();
	void addError(eErrorCategory _category, eErrorCode _code, const std::string& _message);

	uint32_t errorCount() const { return m_errors.size(); }
	const Info& error(uint32_t errorIndex) const;
};


/*
 * Compiler
 *
 */

class ExpressionCompiler
{
	ExpressionErrorReporter errorReport;
	const VariableLayout* layout;

public:
	ExpressionCompiler(const VariableLayout* _layout);

	ExpressionData* compile(const char* expressionText);
	const ExpressionErrorReporter& errors() const { return errorReport; }
};


/*
 * ExpressionEvaluator
 *
 */

class ExpressionEvaluator
{
	const VariablePack* variables;
	ExpressionErrorReporter errorReport;
	std::vector<float> reg;
	eExpType resultType;

	void logDivideByZeroError();

public:
	ExpressionEvaluator(const VariablePack* _variables);

	void evaluate(const ExpressionData* exprData);

	const ExpressionErrorReporter& errors() const { return errorReport; }
	eExpType getResultType() const;
	bool getBoolResult() const;
	float getNumericResult() const;
};


#include "Expression.inl"
