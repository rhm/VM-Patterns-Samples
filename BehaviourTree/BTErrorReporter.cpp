/*
 * Behaviour tree error reporter
 */

#include "stdafx.h"

#include "BTErrorReporter.h"


void BTErrorReporter::reset()
{
	errors.clear();
}

void BTErrorReporter::addError(eBTErrorCategory _category, eBTErrorCode _code, const std::string& _message)
{
	errors.emplace_back(Info{ _category, _code, _message });
}

void BTErrorReporter::combine(const ExpressionErrorReporter& expErrors)
{
	for (uint32_t i = 0; i < expErrors.errorCount(); ++i)
	{
		const ExpressionErrorReporter::Info& err(expErrors.error(i));
		addError(
			static_cast<eBTErrorCategory>(err.category),
			static_cast<eBTErrorCode>(err.code),
			err.message);
	}
}
