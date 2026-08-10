#pragma once
#include "../Lexer/lexer.h"

enum class TokenType {
    Number, Name, Operator, OpenParen, CloseParen, OpenBracket, CloseBracket, EndOfExpr, Error,
    If, Else, While, For,
    OpenBrace, CloseBrace, // {, }
    Semicolon, // ;
    CompareOp, // == != > < >= <=
    Assign, // =
    Comma, // ,
    Print, // print function
    StringLiteral, // string
    FStringLiteral, // f-string, e.g. f"x={x}"
    And, // &&
    Or, // ||
    Not, // !
    CompoundAssign, // +=, -=, *=, /=, %= and ^=
    Boolean, // true/false
    Function, // function,
    Return, // return value
    Global, // global variable
    Local, // local variable
    Void, // for void functions
    Math_const_vars, // constants PI, E
    QuestionMark, // ?
    Colon, // :
    None, // none
    Break, // break;
    Continue, // continue;
    Switch,
    Case,
    Default,
    Line, // for line numbers
    Variable, // "variable", "var" keywords
};

struct Token {
    TokenType type;
    std::string value;
    int lineNumber = 0;
};

class Tokenizer {
    private:
        Lexer& lexer;
        bool isOperator(const std::string& s) const;
    public:
        Tokenizer(Lexer&);
        Token getNextToken();
        
};