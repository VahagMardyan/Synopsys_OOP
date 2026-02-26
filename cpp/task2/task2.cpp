#include "task2.h"

// Numbers

NumberNode::NumberNode(double val) : value(val) {}
double NumberNode::eval(const std::map<std::string, double>& vars) const {
    return value;
}

void NumberNode::print(int indent) const {
    std::cout<<std::string(indent, ' ') << "Number -> " << value << std::endl;
}

// Variables

VariableNode::VariableNode(std::string n) : name(std::move(n)) {}

double VariableNode::eval(const std::map<std::string, double>& vars) const {
    return vars.at(name);
}

void VariableNode::print(int indent) const {
    std::cout<<std::string(indent, ' ')<<"Variable -> "<<name<<std::endl;
}

// Operations
BinaryOpNode ::BinaryOpNode(char o, std::unique_ptr<Node> l, std::unique_ptr<Node> r) : 
op(o), left(std::move(l)), right(std::move(r)) {}

double BinaryOpNode::eval(const std::map<std::string, double>& vars) const {
    double l_val = left -> eval(vars);
    double r_val = right -> eval(vars);
    switch (op) {
        case '+' : return l_val + r_val;
        case '-' : return l_val - r_val;
        case '/' : return l_val / r_val;
        case '*' : return l_val * r_val;
        default : return 0;
    }
}
void BinaryOpNode::print(int indent) const {
    std::cout<<std::string(indent, ' ') << "Op -> " << op << std::endl;
    left -> print(indent + 6);
    right -> print(indent + 4);
}

// Parsing

void Parser::skipWhiteSpace() {
    while(pos < src.size() && isspace(src[pos])) {
        pos++;
    }
}

Parser::Parser(std::string s) : src(std::move(s)) {}

std::unique_ptr<Node> Parser::parseExpression() {
    auto left = parseTerm();
    skipWhiteSpace();
    while(pos < src.size() && (src[pos] == '+' || src[pos] == '-')) {
        char op = src[pos++];
        auto right = parseTerm();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
        skipWhiteSpace();
    }
    return left;
    }
    
std::unique_ptr<Node> Parser::parseTerm() {
    auto left = parseFactor();
    skipWhiteSpace();
    while(pos < src.size() && (src[pos] == '*' || src[pos] == '/')) {
        char op = src[pos++];
        auto right = parseFactor();
        left = std::make_unique<BinaryOpNode>(op, std::move(left), std::move(right));
        skipWhiteSpace();
    }
    return left;
}
std::unique_ptr<Node> Parser::parseFactor() {
    skipWhiteSpace();
    if(src[pos] == '-') {
        if(pos + 1 < src.size() && isdigit(src[pos + 1])) {
            size_t next;
            double val = std::stod(src.substr(pos), &next);
            pos += next;
            return std::make_unique<NumberNode>(val);
        }
        pos++;
        return std::make_unique<BinaryOpNode>('-', std::make_unique<NumberNode>(0), parseFactor());
    }
    if(pos < src.size() && src[pos] == '(') {
        pos++;
        auto node = parseExpression();
        skipWhiteSpace();
        if(pos < src.size() && src[pos] == ')') {
            pos++;
        }
        return node;
    }
    if(pos < src.size() && (isdigit(src[pos]) || src[pos] == '.')) {
        size_t next;
        double val = std::stod(src.substr(pos), &next);
        pos += next;
        return std::make_unique<NumberNode>(val);
    }
    if(pos < src.size() && isalpha(src[pos])) {
        std::string name;
        while(pos < src.size() && (isalnum(src[pos]) || src[pos] == '_')) {
            name += src[pos++];
        }
        return std::make_unique<VariableNode>(name);
    }
    throw std::runtime_error("Unexpexted character at position " + std::to_string(pos));
}
