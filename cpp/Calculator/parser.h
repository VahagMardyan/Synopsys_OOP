#pragma once
#include <stack>
#include <memory>
#include <string>

#include "tokenizer.h"
#include "ast.h"
#include "symbol_table.h"

enum class ParserState {
    ExpectOperand,
    ExpectOperator,
    Done,
    Error
};

class Parser {
    private:
        Tokenizer& tokenizer;
        ParserState state;
        SymbolTable& symTable;
        std::stack<char> operators;
        std::stack<std::shared_ptr<ASTNode>> nodes;
        int precedence(char op) const;
        void processOperatorStack(char currentOp);
    public:
        Parser(Tokenizer& tok, SymbolTable& st);
        std::shared_ptr<ASTNode> parse();
        
};