#include "Machine.hpp"
#include <iostream>
#include <vector>
SimpleLang::Machine::Machine(Compiler::ByteCode const &code)
{
    m_constInts = code.ints;
    m_constStrings = code.ids;
    m_operations = code.operations;
}
void SimpleLang::Machine::addFunction(FunctionValue const &func, std::string const &name)

{
    m_variables[name] = MemoryValue{.type = Type::NativeFunction, .value = func};
}
void SimpleLang::Machine::step()
{
    if (m_programCounter >= m_operations.size())
    {
        return;
    }
    switch ((Operation)m_operations[m_programCounter])
    {
    case Operation::Add:
        _addInt();
        break;
    case Operation::Sub:
        _subInt();
        break;
    case Operation::Call:
        _call();
        break;
    case Operation::Set:
        _set();
        break;
    case Operation::Get:
        _get();
        break;
    case Operation::PushConstInt:
        _pushConstInt();
        break;
    case Operation::PushConstString:
        _pushConstString();
        break;
    case Operation::GetArray:
        _getArray();
        break;
    case Operation::SetArray:
        _setArray();
        break;
    default:
        std::cerr << "Invalid op code: " << (int32_t)m_operations[m_programCounter] << std::endl;
        break;
    }
    m_programCounter++;
}

SimpleLang::MemoryValue *SimpleLang::Machine::getStackTop()

{
    if (m_operationStack.empty())
    {
        return nullptr;
    }
    else
    {
        return &m_operationStack[m_operationStack.size() - 1];
    }
}

SimpleLang::MemoryValue *SimpleLang::Machine::getStackTopAndPop()
{
    if (m_operationStack.empty())
    {
        return nullptr;
    }
    else
    {
        MemoryValue *m = new MemoryValue(m_operationStack[m_operationStack.size() - 1]);
        m_operationStack.pop_back();
        return m;
    }
}

SimpleLang::ArrayNode *SimpleLang::Machine::createArrayOfSize(int32_t size)
{
    ArrayNode *node = new ArrayNode(size);
    m_memoryRoot->push_back(node);
    return node;
}

void SimpleLang::Machine::popStack()
{
    m_operationStack.pop_back();
}

void SimpleLang::Machine::pushToStack(MemoryValue const &val)
{
    m_operationStack.push_back(val);
}

void SimpleLang::Machine::createVariable(std::string const &name, MemoryValue const &value)
{
    m_variables[name] = value;
}

void SimpleLang::Machine::_addInt()
{
    MemoryValue a = m_operationStack[m_operationStack.size() - 1];
    MemoryValue b = m_operationStack[m_operationStack.size() - 2];
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    Value c = std::get<int32_t>(a.value) + std::get<int32_t>(b.value);
    m_operationStack.push_back(MemoryValue{.type = Type::Int, .value = c});
}

void SimpleLang::Machine::_subInt()
{
    MemoryValue a = m_operationStack[m_operationStack.size() - 1];
    MemoryValue b = m_operationStack[m_operationStack.size() - 2];
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    Value c = std::get<int32_t>(b.value) - std::get<int32_t>(a.value);
    m_operationStack.push_back(MemoryValue{.type = Type::Int, .value = c});
}

void SimpleLang::Machine::_set()
{
    // (name val =)
    MemoryValue val = m_operationStack[m_operationStack.size() - 1];
    MemoryValue name = m_operationStack[m_operationStack.size() - 2];
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    StringNode *memStr = dynamic_cast<StringNode *>(std::get<MemoryNode *>(name.value));
    if (memStr != nullptr)
    {
        m_variables[memStr->getString()] = val;
    }
}

void SimpleLang::Machine::_get()
{
    MemoryValue name = m_operationStack[m_operationStack.size() - 1];
    m_operationStack.pop_back();
    assert(std::holds_alternative<MemoryNode *>(name.value));
    StringNode *memStr = dynamic_cast<StringNode *>(std::get<MemoryNode *>(name.value));
    if (memStr != nullptr)
    {
        if (m_variables.count(memStr->getString()) < 1)
        {
            throw RuntimeException(std::string("Attempted to get variable '" + memStr->getString() + "', which doesn't exist"));
        }
        m_operationStack.push_back(m_variables[memStr->getString()]);
    }
}

void SimpleLang::Machine::_call()
{
    MemoryValue func = m_operationStack[m_operationStack.size() - 1];
    m_operationStack.pop_back();
    if (std::holds_alternative<FunctionValue>(func.value))
    {
        std::get<FunctionValue>(func.value)(this);
    }
    else
    {
        throw RuntimeException("Attempted to call a function, but top of the stack doesn't contain a function");
    }
}

void SimpleLang::Machine::_pushConstInt()
{
    m_operationStack.push_back(MemoryValue{.type = Type::Int, .value = m_constInts[(size_t)m_operations[m_programCounter + 1]]});
    m_programCounter++;
}

void SimpleLang::Machine::_pushConstString()
{
    MemoryNode *root = m_memoryRoot;
    std::string &str = m_constStrings[(size_t)m_operations[m_programCounter + 1]];
    StringNode *node = nullptr;
    // avoid making instance for each call, check if there is anything that uses this already
    while (root != nullptr)
    {
        if (StringNode *strNode = dynamic_cast<StringNode *>(root); strNode != nullptr && strNode->getString() == str)
        {
            node = strNode;
            break;
        }
        root = root->getNext();
    }
    if (node == nullptr)
    {
        node = new StringNode(str);
        m_memoryRoot->push_back(node);
    }
    m_programCounter++;
    m_operationStack.push_back(MemoryValue{.type = Type::MemoryObj, .value = node});
}

void SimpleLang::Machine::_getArray()
{
    // for (size_t i = 0; i < m_operationStack.size(); i++)
    // {
    //     std::cout << (m_operationStack.size() - i - 1) << " -> " << typeToString(m_operationStack[i].type) << std::endl;
    // }
    MemoryValue index = m_operationStack[m_operationStack.size() - 2];
    MemoryValue array = m_operationStack[m_operationStack.size() - 1];
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    if (!std::holds_alternative<MemoryNode *>(array.value))
    {
        throw RuntimeException(std::string("Attempted to get array value, but array has instead type: ") + typeToString(array.type));
    }
    if (ArrayNode *arrNode = dynamic_cast<ArrayNode *>(std::get<MemoryNode *>(array.value)); arrNode != nullptr)
    {
        m_operationStack.push_back(*arrNode->getItem(std::get<int32_t>(index.value)));
    }
}

void SimpleLang::Machine::_setArray()
{
    MemoryValue index = m_operationStack[m_operationStack.size() - 3];
    MemoryValue array = m_operationStack[m_operationStack.size() - 2];
    MemoryValue value = m_operationStack[m_operationStack.size() - 1];
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    m_operationStack.pop_back();
    if (!std::holds_alternative<MemoryNode *>(array.value))
    {
        throw RuntimeException(std::string("Attempted to set array value, but array has instead type: ") + typeToString(array.type));
    }
    if (ArrayNode *arrNode = dynamic_cast<ArrayNode *>(std::get<MemoryNode *>(array.value)); arrNode != nullptr)
    {
        arrNode->setItem(std::get<int32_t>(index.value), value);
    }
}

const char *SimpleLang::RuntimeException::what() const throw()
{
    return m_msg.c_str();
}
