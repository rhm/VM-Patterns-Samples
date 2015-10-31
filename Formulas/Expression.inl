/*
 * VariableLayout
 *
 */

inline VariableLayout::VariableLayout()
	: numberCount(0)
	, nameCount(0)
{}

inline bool VariableLayout::variableExists(const Name& variableName) const
{
	auto it = m_layout.find(variableName);
	return it != m_layout.end();
}

inline eExpType VariableLayout::getType(const Name& variableName) const
{
	auto it = m_layout.find(variableName);
	if (it == m_layout.end())
	{
		return eExpType::UNINITIALISED;
	}

	return it->second.type;
}

inline ExpressionSlotIndex VariableLayout::getIndex(const Name& variableName) const
{
	auto it = m_layout.find(variableName);
	if (it == m_layout.end())
	{
		assert(false);
		return 0;
	}

	return it->second.index;
}


/*
 * VariablePack
 */

inline VariablePack::VariablePack(const VariableLayout* _layout, Name initName, float initNumber)
	: layout(_layout)
{
	assert(layout != nullptr);

	floatVars.resize(layout->getNumberCount(), initNumber);
	nameVars.resize(layout->getNameCount(), initName);
}

inline VariablePack::VariablePack(const VariablePack& rhs)
	: floatVars(rhs.floatVars)
	, nameVars(rhs.nameVars)
	, layout(rhs.layout)
{}

inline void VariablePack::setVariable(Name variableName, Name value)
{
	ExpressionSlotIndex idx = layout->getIndex(variableName);
	assert(idx < nameVars.size());
	nameVars[idx] = value;
}

inline void VariablePack::setVariable(Name variableName, float value)
{
	ExpressionSlotIndex idx = layout->getIndex(variableName);
	assert(idx < floatVars.size());
	floatVars[idx] = value;
}

inline void VariablePack::setVariable(ExpressionSlotIndex slotIndex, Name value)
{
	assert(slotIndex < nameVars.size());
	nameVars[slotIndex] = value;
}

inline void VariablePack::setVariable(ExpressionSlotIndex slotIndex, float value)
{
	assert(slotIndex < floatVars.size());
	floatVars[slotIndex] = value;
}

inline Name VariablePack::getVariableName(Name variableName) const
{
	ExpressionSlotIndex idx = layout->getIndex(variableName);
	assert(idx < nameVars.size());
	return nameVars[idx];
}

inline float VariablePack::getVariableNumber(Name variableName) const
{
	ExpressionSlotIndex idx = layout->getIndex(variableName);
	assert(idx < floatVars.size());
	return floatVars[idx];
}

inline Name VariablePack::getVariableName(ExpressionSlotIndex slotIndex) const
{
	assert(slotIndex < nameVars.size());
	return nameVars[slotIndex];
}

inline float VariablePack::getVariableNumber(ExpressionSlotIndex slotIndex) const
{
	assert(slotIndex < floatVars.size());
	return floatVars[slotIndex];
}

/*
 * ExpressionErrorReporter
 */

inline void ExpressionErrorReporter::reset()
{
	m_errors.clear();
}

inline void ExpressionErrorReporter::addError(eErrorCategory _category, eErrorCode _code, const std::string& _message)
{
	m_errors.emplace_back(Info{ _category, _code, _message });
}

inline const ExpressionErrorReporter::Info& ExpressionErrorReporter::error(uint32_t errorIndex) const 
{ 
	assert(errorIndex >= 0 && errorIndex < m_errors.size());
	return m_errors[errorIndex]; 
}

/*
 * ExpressionEvaluator
 */

inline eExpType ExpressionEvaluator::getResultType() const
{
	return resultType;
}

inline bool ExpressionEvaluator::getBoolResult() const
{
	assert(resultType == eExpType::BOOL);
	return reg.size() ? reg[0] != 0.f : false;
}

inline float ExpressionEvaluator::getNumericResult() const
{
	assert(resultType == eExpType::NUMBER);
	return reg[0];
}
