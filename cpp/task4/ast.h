#pragma once

#include <memory>
#include <stdexcept>
#include "symbol_table.h"

class ASTNode {
    public:
        virtual ~ASTNode() = default;
        virtual double evaluate() const = 0;
        virtual void print(int level = 0) const = 0;
};

class NumberNode : public ASTNode {
    private:
        double value;
    public:
        NumberNode(double val) : value(val) {}
        double evaluate() const override {
            return value;
        }
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ') << "Number: " << value << std::endl;
        }
};

class VariableNode : public ASTNode {
    private:
        size_t address;
        const SymbolTable& symTable;
    public:
        VariableNode(size_t addr, const SymbolTable& st) : address(addr), symTable(st) {}
        double evaluate() const override {
            return symTable.getValueByAddress(address);
        }
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ') << "Variable (Address: "<<address<<")"<<std::endl;
        }
};

class BinaryOpNode : public ASTNode {
    private:
        char op;
        std::shared_ptr<ASTNode> left, right;
    public:
        BinaryOpNode(char o, std::shared_ptr<ASTNode> l, std::shared_ptr<ASTNode> r) :
            op(o), left(std::move(l)), right(std::move(r)) {}
        double evaluate() const override {
            double l_val = left -> evaluate();
            double r_val = right -> evaluate();
            switch(op) {
                case '+' : return l_val + r_val; break;
                case '-' : return l_val - r_val; break;
                case '*' : return l_val * r_val; break;
                case '/' : return l_val / r_val; break;
                default: throw std::runtime_error("Unknown operator");
            }
        }
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ')<<"BinaryOp: "<<op<<std::endl;
            left -> print(level + 1);
            right -> print(level + 1);
        }
};

class UnaryOpNode : public ASTNode {
    private:
        char op; // '-' -> minus, '#' -> plus
        std::shared_ptr<ASTNode> child;
    public:
        UnaryOpNode(char o, std::shared_ptr<ASTNode> c) : op(o), child(std::move(c)) {}
        double evaluate() const override {
            double val = child -> evaluate();
            if(op == '_') return -val;
            return val;
        }
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ') << "UnaryOp: "<< (op == '_' ? '-' : '+') <<std::endl;
            child -> print(level + 1);
        }
};

