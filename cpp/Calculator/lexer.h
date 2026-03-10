#pragma once
#include <iostream>
#include <sstream>

class Lexer {
    private:
        std::istream& stream;
        int currentChar;
    public:
        Lexer(std::istream&);
        void advance();
        int peek() const;
        bool isEOF() const;
};