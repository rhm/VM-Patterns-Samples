
#include "stdafx.h"
#include "Name.h"

#include <unordered_set>


class NameTable
{
	friend class Name;

	std::unordered_set<std::string> m_strings;
};


NameTable *Name::sm_NT = nullptr;

inline NameTable* Name::getNameTable()
{
	if (!sm_NT)
	{
		sm_NT = new NameTable;
	}

	return sm_NT;
}

Name::Name()
{
	NameTable* nt = getNameTable();
	auto retval = nt->m_strings.insert("UNINITIALISED");
	p = (*retval.first).c_str();
}

Name::Name(const std::string& s)
{
	NameTable* nt = getNameTable();
	auto retval = nt->m_strings.insert(s);
	p = (*retval.first).c_str();
}

Name::Name(const char *s)
{
	NameTable* nt = getNameTable();
	auto retval = nt->m_strings.emplace(s);
	p = (*retval.first).c_str();
}

