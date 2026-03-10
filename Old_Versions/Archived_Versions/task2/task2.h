#pragma once
#include <iostream>
#include <string>
#include <map>
#include <memory>

struct Node {
    virtual ~Node() = default;
    virtual double eval(const std::map<std::string, double>& vars) const = 0;
    virtual void print(int indent = 0) const = 0;
};

struct NumberNode : public Node {
    double value;
    NumberNode(double);
    double eval(const std::map<std::string, double>&) const override;
    void print(int = 0) const override;
};

struct VariableNode : public Node {
    std::string name;
    VariableNode(std::string);
    double eval(const std::map<std::string, double>&) const override;
    void print(int = 0) const override;
};

struct BinaryOpNode : public Node {
    char op;
    std::unique_ptr<Node> left, right;
    BinaryOpNode(char, std::unique_ptr<Node>, std::unique_ptr<Node>);
    double eval(const std::map<std::string, double>&) const override;
    void print(int indent = 0) const override;
};

class Parser {
    private:
        std::string src;
        size_t pos = 0;
        void skipWhiteSpace();
    public:
        Parser(std::string);
        std::unique_ptr<Node> parseExpression();
        std::unique_ptr<Node> parseTerm();
        std::unique_ptr<Node> parseFactor();
};