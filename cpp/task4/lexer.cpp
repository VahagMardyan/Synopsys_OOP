#include "lexer.h"

Lexer::Lexer(std::istream& s) : stream(s) {
    advance();
}
void Lexer::advance() {
    currentChar = stream.get();
}

int Lexer::peek() const {
    return currentChar;
}

bool Lexer::isEOF() const {
    return currentChar == EOF;
}

