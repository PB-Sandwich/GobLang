#pragma once
#include "Parser.hpp"
#include <vector>
#include <cstdint>
#include "ByteCode.hpp"
namespace SimpleLang::Compiler
{

    class Compiler
    {
    public:
        explicit Compiler(Parser const &parser) : m_parser(parser) {}

        /**
         * @brief Convert given parsed data into reverse polish notation representation of code
         *
         */
        void compile();

        /**
         * @brief Generate byte code that can be used by the interpreter and write it into the `m_byteCode` variable
         * 
         */
        void generateByteCode();

        /**
         * @brief Dump all contents of the stack into the reverse polish notation array
         * 
         */
        void dumpStack();

        /**
         * @brief Get priority for the top item on the stack or -1 if stack is empty
         *
         * @return int32_t Priority
         */
        int32_t getTopStackPriority();

        /**
         * @brief Pop the last item from the stack and return it
         *
         * @return Token* Token from top of the stack or nullptr if stack is empty
         */
        Token *popStack();

        /**
         * @brief Prints the parsed sequence using the token data
         *
         */
        void printCode();

      
        static std::vector<uint8_t> generateGetByteCode(Token *token);
        
        static std::vector<uint8_t> generateSetByteCode(Token *token);

        void appendByteCode(std::vector<uint8_t> const &code);

        ByteCode getByteCode() const { return m_byteCode; }

    private:
        /**
         * @brief code representation in reverse polish notation
         *
         */
        std::vector<Token *> m_code;
        std::vector<uint8_t> m_bytes;

        std::vector<Token *> m_stack;

        ByteCode m_byteCode;
        Parser const &m_parser;
    };

    class CompilerNode
    {
    public:
        virtual std::vector<uint8_t> getOperationGetBytes() = 0;

        virtual std::vector<uint8_t> getOperationSetBytes() { return {}; }
    };

    class OperationCompilerNode : public CompilerNode
    {
    public:
        explicit OperationCompilerNode(std::vector<uint8_t> const &vec) : m_bytes(vec) {}

        std::vector<uint8_t> getOperationGetBytes() override { return m_bytes; }

    private:
        std::vector<uint8_t> m_bytes;
    };

    class TokenCompilerNode : public CompilerNode
    {
    public:
        explicit TokenCompilerNode(Token *token) : m_token(token) {}
        std::vector<uint8_t> getOperationGetBytes() override
        {
            return Compiler::generateGetByteCode(m_token);
        }

        std::vector<uint8_t> getOperationSetBytes() override
        {
            return Compiler::generateSetByteCode(m_token);
        }

    private:
        Token *m_token;
    };
}