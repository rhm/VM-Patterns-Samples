
#pragma once

#include <string>

class NameTable;
class Name;

namespace std
{
	template <>
	struct hash<Name>
	{
		std::size_t operator()(const Name& k) const;
	};
}


class Name
{
	friend struct std::hash<Name>;

	static NameTable *sm_NT;

	const char *p;

	NameTable* getNameTable();

public:
	Name(const Name& rhs)
	{
		p = rhs.p;
	}

	explicit Name(const std::string& s);
	explicit Name(const char *s);
	~Name() {}

	Name& operator=(const Name& rhs)
	{
		p = rhs.p;
		return *this;
	}

	bool operator==(const Name& rhs) const
	{
		return p == rhs.p;
	}

	bool operator!=(const Name& rhs) const
	{
		return p != rhs.p;
	}

	const char *c_str() const { return p; }
};

namespace std
{
	inline std::size_t hash<Name>::operator()(const Name& k) const
	{
		return size_t(k.p);
	}
}

