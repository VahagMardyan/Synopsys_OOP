#pragma once
#include "lexer.h"

enum class TokenType {
    Number, Name, Operator, OpenParen, CloseParen, EndOfExpr, Error
};

struct Token {
    TokenType type;
    std::string value;
};

class Tokenizer {
    private:
        Lexer& lexer;
    public:
        Tokenizer(Lexer&);
        Token getNextToken();
        
};