#pragma once

#include <iostream>
#include "symbol_table.h"

enum class OpCode {
    ADD, SUB, MUL, DIV,
    UNARY,
    LOAD_CONST,
    LOAD_VAR
};

struct Instruction {
    OpCode op;
    int left;
    int right;
    int dest; // index
    double value;
};

class ASTNode {
    public:
        virtual ~ASTNode() = default;        
        virtual void print(int level = 0) const = 0;
        virtual int compile(std::vector<Instruction>& program, int& tempCounter) const = 0;
};

class NumberNode : public ASTNode {
    private:
        double value;
    public:
        NumberNode(double val) : value(val) {}
        int compile(std::vector<Instruction>& program, int& tempCounter) const override {
            int dest = tempCounter++;
            program.push_back({
                OpCode::LOAD_CONST, 0, 0, dest, value
            });
            return dest;
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
        int compile(std::vector<Instruction>& program, int& tempCounter) const override {
            int dest = tempCounter++;
            program.push_back({
                OpCode::LOAD_VAR, (int)address, 0, dest, 0.0
            });
            return dest;
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
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ')<<"BinaryOp: "<<op<<std::endl;
            left -> print(level + 1);
            right -> print(level + 1);
        }
        int compile(std::vector<Instruction>& program, int& tempCounter) const override {
            int l_idx = left -> compile(program, tempCounter);
            int r_idx = right -> compile(program, tempCounter);
            OpCode code;
            switch(op) {
                case '+' : code = OpCode::ADD; 
                break;
                case '-' : code = OpCode::SUB;
                break;
                case '/' : code = OpCode::DIV;
                break;
                case '*' : code = OpCode::MUL;
                break;
                default: break;
            }
            int dest = tempCounter++;
            program.push_back({
                code, l_idx, r_idx, dest, 0.0
            });
            return dest;
        }
};

class UnaryOpNode : public ASTNode {
    private:
        char op; // '-' -> minus, '#' -> plus
        std::shared_ptr<ASTNode> child;
    public:
        UnaryOpNode(char o, std::shared_ptr<ASTNode> c) : op(o), child(std::move(c)) {}
        void print(int level) const override {
            std::cout<<std::string(level * 4, ' ') << "UnaryOp: "<< (op == '_' ? '-' : '+') <<std::endl;
            child -> print(level + 1);
        }
        int compile(std::vector<Instruction>& program, int& tempCounter) const override {
            int c_idx = child -> compile(program, tempCounter);
            int dest = tempCounter++;
            program.push_back({
                OpCode::UNARY, c_idx, 0, dest, 0.0
            });
            return dest;
        }
};

