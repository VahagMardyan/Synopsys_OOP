#include "parser.h"
#include <iostream>

Parser::Parser(Tokenizer& tok, SymbolTable& st) : tokenizer(tok), symTable(st), state(ParserState::ExpectOperand) {}

int Parser::precedence(char op) const {
    if(op == '+' || op == '-') return 1;
    if(op == '*' || op == '/') return 2;
    if(op == '_' || op == '#') return 3; // unary operators
    return 0;
}

void Parser::processOperatorStack(char currentOp) {
    while(!operators.empty() && precedence(operators.top()) >= precedence(currentOp)) {
        char op = operators.top();
        operators.pop();
        if(op == '_' || op == '#') {
            if(nodes.empty()) {
                throw std::runtime_error("Invalid unary expression");
            }
            auto child = nodes.top();
            nodes.pop();
            nodes.push(std::make_shared<UnaryOpNode>(op, child));
        } else {
            if(nodes.size() < 2) {
                throw std::runtime_error("Invalid binary expression");
            }
            auto right = nodes.top();
            nodes.pop();
            auto left = nodes.top();
            nodes.pop();
            nodes.push(std::make_shared<BinaryOpNode>(op, left, right));
        }
    }
}

std::shared_ptr<ASTNode> Parser::parse() {
    double multiplier = 1.0;
    while(state != ParserState::Done && state != ParserState::Error) {
        Token token = tokenizer.getNextToken();
        switch(state) {
            case ParserState::ExpectOperand: {
                if(token.type == TokenType::Number) {
                    double val = std::stod(token.value) * multiplier;
                    nodes.push(std::make_shared<NumberNode>(val));
                    multiplier = 1.0;
                    state = ParserState::ExpectOperator;
                } 
                else if(token.type == TokenType::Name) {
                    size_t address = symTable.getAddress(token.value);
                    if(multiplier == -1.0) {
                        auto var = std::make_shared<VariableNode>(address, symTable);
                        nodes.push(std::make_shared<UnaryOpNode>('_', var));
                        multiplier = 1.0;
                    } else {
                        nodes.push(std::make_shared<VariableNode>(address, symTable));
                    }
                    state = ParserState::ExpectOperator;
                }
                else if(token.type == TokenType::Operator && (token.value == "+" || token.value == "-")) {
                    if(token.value == "-") {
                        multiplier *= -1.0;
                    }
                }
                else if(token.type == TokenType::OpenParen) {
                    operators.push('(');
                } 
                else {
                    std::cerr << "Syntax error: Expected number, variable or '(', but got " << token.value << std::endl;
                    state = ParserState::Error;
                }
            }
            break;
            case ParserState::ExpectOperator: {
                if(token.type == TokenType::Operator) {
                    processOperatorStack(token.value[0]);
                    operators.push(token.value[0]);
                    state = ParserState::ExpectOperand;
                }
                else if(token.type == TokenType::CloseParen) {
                    while(!operators.empty() && operators.top() != '(') {
                        processOperatorStack(operators.top());
                    }
                    if(!operators.empty()) operators.pop();
                }
                else if(token.type == TokenType::EndOfExpr) {
                    processOperatorStack(0);
                    state = ParserState::Done;
                }
                else {
                    std::cerr << "Syntax error: Expected operator or ')', but got " << token.value << std::endl;
                    state = ParserState::Error;
                }
            }
            break;
            default: break;
        }
    }
    if(state == ParserState::Error || nodes.empty()) {
        return nullptr;
    }
    return nodes.top();
}

