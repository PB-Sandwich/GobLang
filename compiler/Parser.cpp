#include "Compiler.hpp"
#include <cctype>
#include <cstring>
#include <sstream>
#include <iostream>
#include <limits>
#include "Parser.hpp"

void SimpleLang::Compiler::Parser::skipWhitespace()
{
    while (std::isspace(*m_rowIt) && m_rowIt != getEndOfTheLine())
    {
        advanceRowIterator(1);
    }
}

void SimpleLang::Compiler::Parser::parse()
{
    std::vector<std::function<Token *(void)>> parsers = {
        std::bind(&Parser::parseKeywords, this),
        std::bind(&Parser::parseInt, this),
        std::bind(&Parser::parseId, this),
        std::bind(&Parser::parseOperators, this),
        std::bind(&Parser::parseSeparators, this),
    };

    while (m_rowIt != getEndOfTheLine() && m_lineIt != m_code.end())
    {
        skipWhitespace();

        Token *token = nullptr;
        for (std::function<Token *(void)> &f : parsers)
        {
            token = f();
            if (token != nullptr)
            {
                m_tokens.push_back(token);
                break;
            }
        }
        if (token == nullptr)
        {
            throw ParsingError(getLineNumber(), getColumnNumber(), "Unknown character sequence");
        }
    }
}

bool SimpleLang::Compiler::Parser::tryKeyword(std::string const &keyword)
{
    for (size_t i = 0; i < keyword.size(); i++)
    {
        if ((m_rowIt + i) == getEndOfTheLine() || *(m_rowIt + i) != keyword[i])
        {
            return false;
        }
    }
    return true;
}

bool SimpleLang::Compiler::Parser::tryOperator(OperatorData const &op)
{
    for (size_t i = 0; i < strnlen(op.symbol, 2); i++)
    {
        if ((m_rowIt + i) == getEndOfTheLine() || *(m_rowIt + i) != op.symbol[i])
        {
            return false;
        }
    }
    return true;
}

SimpleLang::Compiler::KeywordToken *SimpleLang::Compiler::Parser::parseKeywords()
{

    for (std::map<std::string, Keyword>::const_iterator it = Keywords.begin(); it != Keywords.end(); it++)
    {
        if (tryKeyword(it->first))
        {
            size_t row = getLineNumber();
            size_t column = getColumnNumber();
            advanceRowIterator(it->first.size());
            return new KeywordToken(
                row,
                column,
                it->second);
        }
    }
    return nullptr;
}

SimpleLang::Compiler::OperatorToken *SimpleLang::Compiler::Parser::parseOperators()
{
    for (std::vector<OperatorData>::const_iterator it = Operators.begin(); it != Operators.end(); it++)
    {
        if (tryOperator(*it))
        {
            size_t row = getLineNumber();
            size_t column = getColumnNumber();
            size_t offset = strnlen(it->symbol, 2);
            advanceRowIterator(offset);
            return new OperatorToken(row,
                                     column,
                                     it->op);
        }
    }
    return nullptr;
}

SimpleLang::Compiler::IdToken *SimpleLang::Compiler::Parser::parseId()
{
    // must start with a character or underscore
    if (!std::isalpha(*m_rowIt) && (*m_rowIt) != '_')
    {
        return nullptr;
    }
    std::string id;
    size_t offset = 0;
    while ((m_rowIt + offset) != getEndOfTheLine() && (std::isalnum(*(m_rowIt + offset)) || *(m_rowIt + offset) == '_'))
    {
        id.push_back(*(m_rowIt + offset));
        offset++;
    }
    std::vector<std::string>::iterator it = std::find(m_ids.begin(), m_ids.end(), id);
    size_t index = std::string::npos;
    if (it == m_ids.end())
    {
        m_ids.push_back(id);
        index = m_ids.size() - 1;
    }
    else
    {
        index = it - m_ids.begin();
    }
    size_t row = getLineNumber();
    size_t column = getColumnNumber();
    advanceRowIterator(offset);
    return new IdToken(row, column, index);
}

SimpleLang::Compiler::IntToken *SimpleLang::Compiler::Parser::parseInt()
{
    if (!std::isdigit(*m_rowIt))
    {
        return nullptr;
    }
    std::string num;
    size_t offset = 0;
    while ((m_rowIt + offset) != getEndOfTheLine() && std::isdigit(*(m_rowIt + offset)))
    {
        num.push_back(*(m_rowIt + offset));
        offset++;
    }
    size_t index = std::string::npos;
    try
    {
        int32_t numVal = std::stoi(num);
        std::vector<int32_t>::iterator it = std::find(m_ints.begin(), m_ints.end(), numVal);
        if (it == m_ints.end())
        {
            m_ints.push_back(numVal);
            index = m_ints.size() - 1;
        }
        else
        {
            index = it - m_ints.begin();
        }
    }
    catch (std::invalid_argument e)
    {
        return nullptr;
    }
    catch (std::out_of_range e)
    {
        throw ParsingError(
            m_rowIt - m_lineIt->begin(),
            m_lineIt - m_code.begin(),
            "Constant number is too large, valid range is " +
                std::to_string(std::numeric_limits<int32_t>::min()) +
                "< x < " +
                std::to_string(std::numeric_limits<int32_t>::max()));
    }
    size_t row = getLineNumber();
    size_t column = getColumnNumber();
    advanceRowIterator(offset);
    return new IntToken(row, column, index);
}

SimpleLang::Compiler::SeparatorToken *SimpleLang::Compiler::Parser::parseSeparators()
{

    for (std::vector<SeparatorData>::const_iterator it = Separators.begin(); it != Separators.end(); it++)
    {
        if (*m_rowIt == it->symbol)
        {
            size_t row = getLineNumber();
            size_t column = getColumnNumber();
            advanceRowIterator(1);
            return new SeparatorToken(row, column, it->separator);
        }
    }
    return nullptr;
}

void SimpleLang::Compiler::Parser::advanceRowIterator(size_t offset, bool stopAtEndOfTheLine)
{
    size_t currentOffset = 0;
    for (; currentOffset < offset && m_rowIt != getEndOfTheLine(); currentOffset++)
    {
        m_rowIt++;
    }
    if (m_rowIt == getEndOfTheLine())
    {
        m_lineIt++;
        if (m_lineIt == m_code.end())
        {
            return;
        }
        m_rowIt = m_lineIt->begin();
        if (currentOffset < offset && !stopAtEndOfTheLine)
        {
            advanceRowIterator(offset - currentOffset, stopAtEndOfTheLine);
        }
    }
}

size_t SimpleLang::Compiler::Parser::getLineNumber() const
{
    return m_lineIt - m_code.begin();
}

size_t SimpleLang::Compiler::Parser::getColumnNumber() const
{
    return m_rowIt - m_lineIt->begin();
}

void SimpleLang::Compiler::Parser::printInfoTable()
{
    for (size_t i = 0; i < m_ids.size(); i++)
    {
        std::cout << "W" << i << ": " << m_ids[i] << std::endl;
    }

    for (size_t i = 0; i < m_ints.size(); i++)
    {
        std::cout << "NUM" << i << ": " << m_ints[i] << std::endl;
    }
}

void SimpleLang::Compiler::Parser::printCode()
{
    for (std::vector<Token *>::iterator it = m_tokens.begin(); it != m_tokens.end(); it++)
    {
        std::cout << (*it)->toString() << " ";
    }
    std::cout << std::endl;
}
SimpleLang::Compiler::Parser::Parser(std::vector<std::string> const &code) : m_code(code)
{
    m_lineIt = m_code.begin();
    m_rowIt = m_lineIt->begin();
}

SimpleLang::Compiler::Parser::Parser(std::string const &code)
{
    std::stringstream stream(code);
    std::string to;
    while (std::getline(stream, to, '\n'))
    {
        m_code.push_back(to);
    }
    m_lineIt = m_code.begin();
    m_rowIt = m_lineIt->begin();
}

SimpleLang::Compiler::Parser::~Parser()
{
    for (size_t i = 0; i < m_tokens.size(); i++)
    {
        delete m_tokens[i];
    }
    m_tokens.clear();
}

const char *SimpleLang::Compiler::ParsingError::what() const throw()
{
    return m_full.c_str();
}

SimpleLang::Compiler::ParsingError::ParsingError(size_t row, size_t column, std::string const &msg) : m_row(row), m_column(column), m_message(msg)
{
    m_full = ("Error at line " + std::to_string(m_row) + " row " + std::to_string(m_column) + ": " + m_message);
}
