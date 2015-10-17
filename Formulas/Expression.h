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


class ExpressionCompiler
{




};




