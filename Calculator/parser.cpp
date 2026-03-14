#include "parser.h"
#include <iostream>

Parser::Parser(Tokenizer& tok, SymbolTable& st) : tokenizer(tok), symTable(st), state(ParserState::ExpectOperand) {}

int Parser::precedence(char op) const {
    if(op == '+' || op == '-') return 1;
    if(op == '*' || op == '/') return 2;
    if(op == '_' || op == '#') return 3; // unary operators
    return 0;
}

void Parser::createNodeFromOp(char op) {
    if(op == '_' || op == '#') {
        if(nodes.empty()) {
            throw std::runtime_error("Invalid unary expression");
        }
        auto child = nodes.top();
        nodes.pop();
        if(op == '_') {
            auto numNode = std::dynamic_pointer_cast<NumberNode>(child);
            if(numNode) {
                nodes.push(std::make_shared<NumberNode>(-numNode->getValue()));
            } else {
                nodes.push(std::make_shared<UnaryOpNode>('_', child));
            }
        } else {
            nodes.push(child);
        }
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

void Parser::processOperatorStack(char currentOp) {
    while(!operators.empty() && 
           operators.top() != '(' && precedence(operators.top()) >= precedence(currentOp)) {

        char op = operators.top();
        operators.pop();
        createNodeFromOp(op);
    }
}

std::shared_ptr<ASTNode> Parser::parse() {
    while(state != ParserState::Done && state != ParserState::Error) {
        Token token = tokenizer.getNextToken();
        switch(state) {
            case ParserState::ExpectOperand: {
                if(token.type == TokenType::Number) {
                    nodes.push(std::make_shared<NumberNode>(std::stod(token.value)));
                    state = ParserState::ExpectOperator;
                } else if(token.type == TokenType::Name) {
                    size_t address = symTable.getAddress(token.value);
                    nodes.push(std::make_shared<VariableNode>(address, symTable));
                    state = ParserState::ExpectOperator;
                } else if(token.type == TokenType::Operator) {
                    if(token.value == "-") {
                        if(!operators.empty() && operators.top() == '_') {
                            operators.pop();
                        } else if(!operators.empty() && operators.top() == '#') {
                            operators.pop();
                            operators.push('_');
                        } else {
                            operators.push('_');
                        }
                    } else if(token.value == "+") {
                        if(operators.empty() || (operators.top() != '_' && operators.top() != '#')) {
                            operators.push('#');
                        }
                    } else {
                        state = ParserState::Error;
                    }
                } else if(token.type == TokenType::OpenParen) {
                    operators.push('(');
                } else {
                    state = ParserState::Error;
                }
            }
            break;

            case ParserState::ExpectOperator: {
                if(token.type == TokenType::Operator) {
                    processOperatorStack(token.value[0]);
                    operators.push(token.value[0]);
                    state = ParserState::ExpectOperand;
                } else if(token.type == TokenType::Name || token.type == TokenType::OpenParen) {
                    processOperatorStack('*');
                    operators.push('*');
                    if(token.type == TokenType::Name) {
                        size_t address = symTable.getAddress(token.value);
                        nodes.push(std::make_shared<VariableNode>(address, symTable));
                        state = ParserState::ExpectOperator;
                    } else {
                        operators.push('(');
                        state = ParserState::ExpectOperand;
                    }
                } else if(token.type == TokenType::CloseParen) {
                    while(!operators.empty() && operators.top() != '(') {
                        createNodeFromOp(operators.top());
                        operators.pop();
                    }
                    if(!operators.empty()) {
                        operators.pop();
                    }
                } else if(token.type == TokenType::EndOfExpr) {
                    processOperatorStack(0);
                    state = ParserState::Done;
                }
            }
            break;
            default: break;
        }
    }
    return nodes.empty() ? nullptr : nodes.top();
}

