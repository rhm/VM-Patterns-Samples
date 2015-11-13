/*
 * Behaviour tree error reporter
 */

#pragma once

#include "Expression.h"


enum class eBTErrorCategory
{
	UNINITIALISED = eErrorCategory::MAX,

	ExpressionType,

	MAX
};

enum class eBTErrorCode
{
	UNINITIALISED = eErrorCode::MAX,

	ConditionTypeNotBool,

	MAX
};


class BTErrorReporter
{
public:
	struct Info
	{
		eBTErrorCategory category;
		eBTErrorCode code;
		std::string message;
	};

private:
	std::vector<Info> errors;

public:
	BTErrorReporter() {}
	~BTErrorReporter() {}

	void reset();
	void addError(eBTErrorCategory _category, eBTErrorCode _code, const std::string& _message);
	void combine(const ExpressionErrorReporter& expErrors);

	uint32_t errorCount() const { return errors.size(); }
	const Info& error(uint32_t errorIndex) const;
};
