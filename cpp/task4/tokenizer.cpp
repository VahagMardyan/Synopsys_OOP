#include "tokenizer.h"

Tokenizer::Tokenizer(Lexer& l) : lexer(l) {}

Token Tokenizer::getNextToken() {
    while(!lexer.isEOF() && isspace(lexer.peek())) {
        lexer.advance();
    }
    if(lexer.isEOF()) {
        return {
            TokenType::EndOfExpr, ""
        };
    }
    char current = lexer.peek();
    if(isdigit(current) || current == '.') {
        std::string val;
        bool hasDot = false;
        while(!lexer.isEOF() && (isdigit(lexer.peek()) || lexer.peek() == '.') ) {
            if(lexer.peek() == '.') {
                if(hasDot) break;
                hasDot = true;
            }
            val += (char)lexer.peek();
            lexer.advance();
        }
        return {
            TokenType::Number, val
        };
    }
    if(isalnum(current) || current == '_') {
        std::string name;
        while(!lexer.isEOF() && ( isalnum(lexer.peek()) || lexer.peek() == '_')) {
            name += (char)lexer.peek();
            lexer.advance();
        }
        return {
            TokenType::Name, name
        };
    }
    lexer.advance();
    if(current == '+' || current == '-' || current == '/' || current == '*') {
        return {
            TokenType::Operator, std::string(1, current)
        };
    }
    if(current == '(') {
        return {
            TokenType::OpenParen, "("
        };
    }
    if(current == ')') {
        return {
            TokenType::CloseParen, ")"
        };
    }
    return {
        TokenType::Error, std::string(1, current)
    };
}
