#include "CompilerToken.hpp"

std::string GobLang::Compiler::FunctionCallToken::toString()
{
    return "CALL_" + std::to_string(getArgCount()) + (m_usesLocalFunc ? (std::string("_LOCAL") + std::to_string(m_funcId)) : "");
}

void GobLang::Compiler::MultiArgToken::increaseArgCount()
{
    m_argCount++;
}

std::string GobLang::Compiler::LocalVarShrinkToken::toString()
{
    return "SHRINK_BY" + std::to_string(m_amount);
}

std::string GobLang::Compiler::ReturnToken::toString()
{
    return std::string("RET") + (m_hasVal ? "_VAL" : "");
}

std::string GobLang::Compiler::MultiArgToken::toString()
{
    return "MULTI_ARG_NOCOMPILE_" + std::to_string(m_argCount);
}

std::string GobLang::Compiler::ArrayCreationToken::toString()
{
    return "ARRAY_SIZE_" + std::to_string(getArgCount());
}
