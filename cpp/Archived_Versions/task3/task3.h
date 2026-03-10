#pragma once
#include <string>
#include <map>
#include <vector>

enum class TokenType {
    Number, Identifier, Operator, LParen, RParen, End, Error
};

struct Token {
    TokenType type;
    std::string value;
};

class Lexer {
    public:
        explicit Lexer(const std::string& in);
        Token next();
    private:
        enum class State { Start, Number, Identifier, Done, Error };
        std::string input;
        size_t pos = 0;
};

class Evaluator {
    public:
        double evaluate(const std::string& expr,const std::map<std::string, double>& vars);
    private:
        std::vector<Token> infixToPostfix(Lexer& lexer);
        double evalPostfix(const std::vector<Token>& postfix, const std::map<std::string,double>& vars);
};