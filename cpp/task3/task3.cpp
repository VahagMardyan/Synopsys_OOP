#include "task3.h"
#include <cctype>
#include <stdexcept>
#include <stack>

Lexer::Lexer(const std::string& in) : input(in), pos(0) {}

Token Lexer::next() {
    State state = State::Start;
    std::string buffer;

    while(true) {
        char ch = (pos < input.size()) ? input[pos] : '\0';
        bool atEnd = (pos >= input.size());

        switch(state) {
            case State::Start:
                if(atEnd) return {TokenType::End,""};
                if(isspace(ch)) { pos++; continue; }
                else if(isdigit(ch) || ch=='.') { buffer+=ch; state=State::Number; pos++; }
                else if(isalpha(ch) || ch=='_') { buffer+=ch; state=State::Identifier; pos++; }
                else if(ch=='+'||ch=='-'||ch=='*'||ch=='/') { pos++; return {TokenType::Operator,std::string(1,ch)}; }
                else if(ch=='(') { pos++; return {TokenType::LParen,"("}; }
                else if(ch==')') { pos++; return {TokenType::RParen,")"}; }
                else { buffer+=ch; state=State::Error; }
                break;

            case State::Number:
                if(atEnd || !(isdigit(ch) || ch=='.')) {
                    return {TokenType::Number, buffer};
                } else { buffer+=ch; pos++; }
                break;

            case State::Identifier:
                if(atEnd || !(isalnum(ch) || ch=='_')) {
                    return {TokenType::Identifier, buffer};
                } else { buffer+=ch; pos++; }
                break;

            default:
                return {TokenType::Error, buffer};
        }
    }
}

int precedence(const std::string& op) {
    if(op == "+" || op == "-") return 1;
    if(op == "*" || op == "/") return 2;
    if(op == "~") return 3; // unary minus - highest precedence
    return 0;
}

std::vector<Token> Evaluator::infixToPostfix(Lexer& lexer) {
    std::vector<Token> outputQueue;
    std::stack<Token> opStack;

    Token token = lexer.next();
    Token prevToken = {TokenType::Operator, ""};

    while(token.type != TokenType::End) {
        if(token.type == TokenType::Error) {
            throw std::runtime_error("Unknown token: " + token.value);
        }

        if(token.type == TokenType::Number || token.type == TokenType::Identifier) {
            outputQueue.push_back(token);
        } else if(token.type == TokenType::Operator) {
            // unary minus detection
            bool isUnary = (token.value == "-") && (
                prevToken.type == TokenType::Operator ||
                prevToken.type == TokenType::LParen ||
                prevToken.type == TokenType::End
            );

            if(isUnary) {
                // Unary minus: right-associative, highest precedence
                // Pop any other ~ operators (chained unary), but nothing else
                opStack.push({TokenType::Operator, "~"});
            } else {
                // For binary operators, pop if stack top has higher precedence
                // OR equal precedence (left-associative).
                // ~ (unary) has precedence 3 but is right-associative — do pop it
                // since it must apply to operand before binary op takes effect.
                while(!opStack.empty() &&
                       opStack.top().type == TokenType::Operator &&
                       opStack.top().type != TokenType::LParen) {
                    int topPrec = precedence(opStack.top().value);
                    int curPrec = precedence(token.value);
                    bool topIsUnary = (opStack.top().value == "~");
                    
                    if(topPrec > curPrec || (topPrec == curPrec && !topIsUnary)) {
                        outputQueue.push_back(opStack.top());
                        opStack.pop();
                    } else {
                        break;
                    }
                }
                opStack.push(token);
            }
        } else if(token.type == TokenType::LParen) {
            opStack.push(token);
        } else if(token.type == TokenType::RParen) {
            bool foundLParen = false;
            while(!opStack.empty()) {
                if(opStack.top().type == TokenType::LParen) {
                    foundLParen = true;
                    opStack.pop();
                    break;
                }
                outputQueue.push_back(opStack.top());
                opStack.pop();
            }
            if(!foundLParen) {
                throw std::runtime_error("Mismatched parentheses");
            }
        }

        prevToken = token;
        token = lexer.next();
    }

    while(!opStack.empty()) {
        if(opStack.top().type == TokenType::LParen || opStack.top().type == TokenType::RParen) {
            throw std::runtime_error("Mismatched parentheses");
        }
        outputQueue.push_back(opStack.top());
        opStack.pop();
    }
    return outputQueue;
}

double Evaluator::evalPostfix(const std::vector<Token>& postfix, const std::map<std::string, double>& vars) {
    std::stack<double> evalStack;

    for(auto& t : postfix) {
        if(t.type == TokenType::Number) {
            evalStack.push(std::stod(t.value));
        } else if(t.type == TokenType::Identifier) {
            if(!vars.count(t.value)) {
                throw std::runtime_error("Unknown variable: " + t.value);
            }
            evalStack.push(vars.at(t.value));
        } else if(t.type == TokenType::Operator) {
            if(t.value == "~") {
                if(evalStack.empty()) throw std::runtime_error("Invalid expression");
                double a = evalStack.top(); evalStack.pop();
                evalStack.push(-a);
                continue;
            }
            if(evalStack.size() < 2) {
                throw std::runtime_error("Invalid expression: not enough operands for operator '" + t.value + "'");
            }
            double b = evalStack.top(); evalStack.pop();
            double a = evalStack.top(); evalStack.pop();
            double res = 0;
            if(t.value == "+")      res = a + b;
            else if(t.value == "-") res = a - b;
            else if(t.value == "*") res = a * b;
            else if(t.value == "/") {
                if(b == 0) throw std::runtime_error("Division by zero");
                res = a / b;
            }
            evalStack.push(res);
        }
    }

    if(evalStack.empty()) {
        throw std::runtime_error("Invalid expression: empty result");
    }
    if(evalStack.size() != 1) {
        throw std::runtime_error("Invalid expression: too many values");
    }
    return evalStack.top();
}

double Evaluator::evaluate(const std::string& expr, const std::map<std::string, double>& vars) {
    if(expr.empty()) {
        throw std::runtime_error("Empty expression");
    }
    Lexer lexer(expr);
    auto postfix = infixToPostfix(lexer);
    return evalPostfix(postfix, vars);
}