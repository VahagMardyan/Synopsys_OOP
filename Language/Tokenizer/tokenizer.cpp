#include "tokenizer.h"
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <unordered_map>

const std::unordered_map<std::string, TokenType> operations = {
    {"+", TokenType::Operator}, {"-", TokenType::Operator},
    {"*", TokenType::Operator}, {"/", TokenType::Operator},
    {"&", TokenType::Operator}, {"|", TokenType::Operator},
    {"^", TokenType::Operator}, {"%", TokenType::Operator},
    {">>", TokenType::Operator}, {"<<", TokenType::Operator},
    {"**", TokenType::Operator}, {"//", TokenType::Operator},
    {"%/", TokenType::Operator}, {"~", TokenType::Operator},
};

namespace {
    std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return s;
    }

    const std::unordered_set<std::string> BuiltIns = {
        "sin", "cos", "tan",
        "asin", "acos", "atan", "atan2",
        "sqrt", "exp", "log", "ln", "log10",
        "ceil", "floor", "abs", "round",
        "fmod", "cbrt", "log2", "pow", "log_ab", // log(b)/log(a)
        "input", // user-input
        "ord", "chr", "type",
        "bin", "oct", "hex", "dec", "random",
        "array", "array_push", "array_pop", "array_insert", "array_remove",
        "number", "string", // type constructors: number("55") -> 55, string(123) -> "123"
    };
}

Tokenizer::Tokenizer(Lexer& l) : lexer(l) {}

bool Tokenizer::isOperator(const std::string& s) const {
    return operations.find(s) != operations.end();
}

Token Tokenizer::getNextToken() {
    // 1. Skip whitespace and comments
    while(!lexer.isEOF()) {
        char current = static_cast<char>(lexer.peek());
        if(isspace(current)) { lexer.advance(); continue; }
        if(current == '#') {
            lexer.advance();
            if(!lexer.isEOF() && lexer.peek() == '*') {
                lexer.advance();
                while(!lexer.isEOF()) {
                    if(lexer.peek() == '*') {
                        lexer.advance();
                        if(!lexer.isEOF() && lexer.peek() == '#') { lexer.advance(); break; }
                    } else { lexer.advance(); }
                }
            } else {
                while(!lexer.isEOF() && lexer.peek() != '\n') lexer.advance();
            }
            continue;
        }
        break;
    }

    lexer.markTokenStart();

    if(lexer.isEOF()) return {TokenType::EndOfExpr, "", lexer.getLineNumber()};

    char current = static_cast<char>(lexer.peek());

    // 2. String literal
    if(current == '"' || current == '\'') {
        char openQuote = current;
        lexer.advance();
        std::string str;
        while(!lexer.isEOF() && lexer.peek() != openQuote) {
            char c = (char)lexer.peek();
            if(c == '\\') {
                lexer.advance();
                char esc = (char)lexer.peek();
                switch(esc) {
                    case 'n':  str += '\n'; break;
                    case 't':  str += '\t'; break;
                    case '"':  str += '"';  break;
                    case '\\': str += '\\'; break;
                    default:   str += esc;  break;
                }
            } else { str += c; }
            lexer.advance();
        }
        if(!lexer.isEOF()) lexer.advance();
        return {TokenType::StringLiteral, str, lexer.getLineNumber()};
    }

    // 3. Punctuation
    if(current == '(') { lexer.advance(); return {TokenType::OpenParen,  "(", lexer.getLineNumber()}; }
    if(current == ')') { lexer.advance(); return {TokenType::CloseParen,  ")", lexer.getLineNumber()}; }
    if(current == '[') { lexer.advance(); return {TokenType::OpenBracket, "[", lexer.getLineNumber()}; }
    if(current == ']') { lexer.advance(); return {TokenType::CloseBracket, "]", lexer.getLineNumber()}; }
    if(current == '{') { lexer.advance(); return {TokenType::OpenBrace,   "{", lexer.getLineNumber()}; }
    if(current == '}') { lexer.advance(); return {TokenType::CloseBrace,  "}", lexer.getLineNumber()}; }
    if(current == ';') { lexer.advance(); return {TokenType::Semicolon,   ";", lexer.getLineNumber()}; }
    if(current == ',') { lexer.advance(); return {TokenType::Comma,       ",", lexer.getLineNumber()}; }
    if(current == '?') { lexer.advance(); return {TokenType::QuestionMark, "?", lexer.getLineNumber()}; }
    if(current == ':') { lexer.advance(); return {TokenType::Colon, ":", lexer.getLineNumber()}; }

    // 4. Numbers
    // Supports: 1_000_000 (underscore separator, stripped)
    // 1e6, 1e+6, 1.5e-3 (scientific notation)
    if(isdigit(current) || current == '.') {
        std::string val;
        bool hasDot = false;
        while(!lexer.isEOF() && (isdigit(lexer.peek()) || lexer.peek() == '.' || lexer.peek() == '_')) {
            if(lexer.peek() == '_') { lexer.advance(); continue; } // strip separator
            if(lexer.peek() == '.') { if(hasDot) break; hasDot = true; }
            val += (char)lexer.peek();
            lexer.advance();
        }
        // Scientific notation: e/E followed by optional +/- and digits
        if(!lexer.isEOF() && (lexer.peek() == 'e' || lexer.peek() == 'E')) {
            val += (char)lexer.peek();
            lexer.advance();
            if(!lexer.isEOF() && (lexer.peek() == '+' || lexer.peek() == '-')) {
                val += (char)lexer.peek();
                lexer.advance();
            }
            while(!lexer.isEOF() && isdigit(lexer.peek())) {
                val += (char)lexer.peek();
                lexer.advance();
            }
        }
        return {TokenType::Number, val, lexer.getLineNumber()};
    }

    // 5. Keywords and identifiers
    if(isalpha(current) || current == '_') {
        std::string name;
        while(!lexer.isEOF() && (isalnum(lexer.peek()) || lexer.peek() == '_')) {
            name += (char)lexer.peek();
            lexer.advance();
        }
        const std::string lowered = toLower(name);
        if(lowered == "if")    return {TokenType::If,    lowered, lexer.getLineNumber()};
        if(lowered == "else")  return {TokenType::Else,  lowered, lexer.getLineNumber()};
        if(lowered == "while") return {TokenType::While, lowered, lexer.getLineNumber()};
        if(lowered == "for")   return {TokenType::For,   lowered, lexer.getLineNumber()};
        if(lowered == "print") return {TokenType::Print, lowered, lexer.getLineNumber()};
        if(lowered == "and")   return {TokenType::And,   lowered, lexer.getLineNumber()};
        if(lowered == "or")    return {TokenType::Or,    lowered, lexer.getLineNumber()};
        if(lowered == "not")   return {TokenType::Not,   lowered, lexer.getLineNumber()};
        if(lowered == "true" || lowered == "false") return {TokenType::Boolean, lowered, lexer.getLineNumber()};
        if(lowered == "function") return {TokenType::Function, lowered, lexer.getLineNumber()};
        if(lowered == "return") return {TokenType::Return, lowered, lexer.getLineNumber()};
        if(lowered == "local") return {TokenType::Local, lowered, lexer.getLineNumber()};
        if(lowered == "global") return {TokenType::Global, lowered, lexer.getLineNumber()};
        if(lowered == "void") return {TokenType::Void, lowered, lexer.getLineNumber()};
        if(lowered == "m_pi" || lowered == "m_e" || lowered == "m_inf" || lowered == "m_max")
            return {TokenType::Math_const_vars, lowered, lexer.getLineNumber()};
        if(BuiltIns.find(lowered) != BuiltIns.end()) return {TokenType::Name, lowered, lexer.getLineNumber()};
        if(lowered == "none") return { TokenType::None, lowered ,lexer.getLineNumber()};
        if(lowered == "break") return { TokenType::Break, lowered , lexer.getLineNumber()};
        if(lowered == "continue") return { TokenType::Continue, lowered , lexer.getLineNumber()};
        if(lowered == "switch")  return {TokenType::Switch, lowered, lexer.getLineNumber()};
        if(lowered == "case") return { TokenType::Case, lowered, lexer.getLineNumber()};
        if(lowered == "default") return { TokenType::Default, lowered, lexer.getLineNumber()};
        if(lowered == "variable" || lowered == "var") return { TokenType::Variable, lowered, lexer.getLineNumber() };
        return {TokenType::Name, name, lexer.getLineNumber()};
    }

    // 6. Operators
    std::string op;
    op += current;
    lexer.advance();
    if(!lexer.isEOF()) {
        char next = static_cast<char>(lexer.peek());
        if((current == '=' && next == '=') || // ==
           (current == '!' && next == '=') || // !=
           (current == '<' && next == '=') || // <=
           (current == '>' && next == '=') || // >=
           (current == '<' && next == '<') || // <<
           (current == '>' && next == '>') || // >>
           (current == '+' && next == '=') || // +=
           (current == '-' && next == '=') || // -=
           (current == '/' && next == '=') || // /=
           (current == '*' && next == '=') || // *=
           (current == '%' && next == '=') || // %=
           (current == '^' && next == '=') || // ^=
           (current == '*' && next == '*') || // **
           (current == '/' && next == '/') || // floor division operator (//) 
           (current == '%' && next == '/')) { // fractional division operator (a%/b = a/b - a//b). Fractional part
            op += next;
            lexer.advance();
        }
    }
    if(op == "=")  return {TokenType::Assign, op, lexer.getLineNumber()};
    if(op == "==" || op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=") {
        return {TokenType::CompareOp, op, lexer.getLineNumber()};
    }
    
    if(op == "+=" || op == "-=" || op == "/=" || op == "*=" || op == "%=" || op == "^=") {
        return {TokenType::CompoundAssign, op, lexer.getLineNumber()};
    }
    if(isOperator(op)) return {TokenType::Operator, op, lexer.getLineNumber()};

    return {TokenType::Error, op, lexer.getLineNumber()};
}