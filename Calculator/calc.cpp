#include "calc.h"

void disassemble(const std::vector<Instruction>& program) {
    std::cout<<"\n[SYMBOLIC DISASSEMBLY]" << std::endl;
    std::cout << std::left << std::setw(6)  << "Addr" 
              << std::setw(12) << "OpCode" 
              << std::setw(6)  << "L" 
              << std::setw(6)  << "R" 
              << std::setw(6)  << "Dst" 
              << "Value" << std::endl;
    std::cout << std::string(45, '-') << std::endl;
    for(size_t i=0;i<program.size(); ++i) {
        const auto& inst = program[i];
        std::cout<<"[" << i <<"]  ";
        switch(inst.op) {
            case OpCode::LOAD_CONST:
                std::cout << std::setw(12) << "LOAD_CONST" << "-     -     " 
                          << std::setw(6) << inst.dest << inst.value;
                break;
            case OpCode::LOAD_VAR:
                std::cout << std::setw(12) << "LOAD_VAR" 
                          << std::setw(6) << inst.left 
                          << std::setw(6) << "-"
                          << std::setw(6) << inst.dest
                          << "&(var_ref)";
                break;
            case OpCode::ADD:
                std::cout << std::setw(12) << "ADD" << std::setw(6) << inst.left 
                          << std::setw(6) << inst.right << std::setw(6) << inst.dest << "?";
                break;
            case OpCode::SUB:
                std::cout << std::setw(12) << "SUB" << std::setw(6) << inst.left 
                          << std::setw(6) << inst.right << std::setw(6) << inst.dest << "?";
                break;
            case OpCode::MUL:
                std::cout << std::setw(12) << "MUL" << std::setw(6) << inst.left 
                          << std::setw(6) << inst.right << std::setw(6) << inst.dest << "?";
                break;
            case OpCode::DIV:
                std::cout << std::setw(12) << "DIV" << std::setw(6) << inst.left 
                          << std::setw(6) << inst.right << std::setw(6) << inst.dest << "?";
                break;
            case OpCode::UNARY:
                std::cout << std::setw(12) << "NEG" << std::setw(6) << inst.left 
                          << "-     " << std::setw(6) << inst.dest << "-";
                break;
        }
        std::cout<<std::endl;
    }
    std::cout << std::string(45, '-') << std::endl;
}

double runVM(const std::vector<Instruction>& program, const SymbolTable& symTable, int tempSize, int finalIdx) {
    if(program.empty()) return 0.0;
    
    std::vector<double> ev(tempSize, 0.0);
    
    for(const auto& inst : program) {
        switch(inst.op) {
            case OpCode::LOAD_CONST : ev[inst.dest] = inst.value;
            break;
            case OpCode::LOAD_VAR : ev[inst.dest] = symTable.getValueByAddress(inst.left);
            break;
            case OpCode::ADD : ev[inst.dest] = ev[inst.left] + ev[inst.right];
            break;
            case OpCode::SUB : ev[inst.dest] = ev[inst.left] - ev[inst.right];
            break;
            case OpCode::DIV : 
                if(ev[inst.right] == 0) throw std::runtime_error("Division by zero in VM");
                ev[inst.dest] = ev[inst.left] / ev[inst.right];
            break;
            case OpCode::MUL : ev[inst.dest] = ev[inst.left] * ev[inst.right];
            break;
            case OpCode::UNARY : ev[inst.dest] = -ev[inst.dest];
            break;
            default: throw std::runtime_error("Unknown OpCode in VM");
        }
    }
    return ev[finalIdx];
}