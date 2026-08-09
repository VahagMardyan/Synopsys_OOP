#include "debugger.h"
#include "vm.h"
#include "../Compiler/compiler.h"
#include <iostream>
#include <iomanip>

void Debugger::printInstructionCompact(size_t pc) const {
    if (pc >= vm.getProgram().size()) return;
    const Instruction& inst = vm.getProgram()[pc];
    OpCode op = static_cast<OpCode>(inst.op);
    std::cout << "[" << pc << "] ";
    std::cout << std::left;
    
    switch (op) {
        case OpCode::ADD:         std::cout << "ADD r" << inst.dst << " = r" << inst.left << " + r" << inst.right; break;
        case OpCode::MOV:         std::cout << "MOV r" << inst.dst << " = r" << inst.left; break;
        case OpCode::SUB:         std::cout << "SUB r" << inst.dst << " = r" << inst.left << " - r" << inst.right; break;
        case OpCode::AND:         std::cout << "AND r" << inst.dst << " = r" << inst.left << " & r" << inst.right; break;
        case OpCode::OR:          std::cout << "OR r" << inst.dst << " = r" << inst.left << " | r" << inst.right; break;
        case OpCode::XOR:         std::cout << "XOR r" << inst.dst << " = r" << inst.left << " ^ r" << inst.right; break;
        case OpCode::NOT:         std::cout << "NOT r" << inst.dst << " = ~r" << inst.left; break;
        case OpCode::SLL:         std::cout << "SLL r" << inst.dst << " = r" << inst.left << " << r" << inst.right; break;
        case OpCode::SRL:         std::cout << "SRL r" << inst.dst << " = r" << inst.left << " >> r" << inst.right; break;
        case OpCode::SRA:         std::cout << "SRA r" << inst.dst << " = r" << inst.left << " >>> r" << inst.right; break;
        case OpCode::SLT:         std::cout << "SLT r" << inst.dst << " = r" << inst.left << " < r" << inst.right; break;
        case OpCode::SLTU:        std::cout << "SLTU r" << inst.dst << " = r" << inst.left << " < r" << inst.right; break;
        
        case OpCode::MUL:         std::cout << "MUL r" << inst.dst << " = r" << inst.left << " * r" << inst.right; break;
        case OpCode::DIV:         std::cout << "DIV r" << inst.dst << " = r" << inst.left << " / r" << inst.right; break;
        case OpCode::MODULO:      std::cout << "MOD r" << inst.dst << " = r" << inst.left << " % r" << inst.right; break;
        case OpCode::POW:         std::cout << "POW r" << inst.dst << " = r" << inst.left << " ** r" << inst.right; break;
        case OpCode::FLOOR_DIV:   std::cout << "FLOOR_DIV r" << inst.dst << " = r" << inst.left << " // r" << inst.right; break;
        case OpCode::FRAC_DIV:    std::cout << "FRAC_DIV r" << inst.dst << " = frac(r" << inst.left << " / r" << inst.right << ")"; break;
        case OpCode::UNARY:       std::cout << "NEG r" << inst.dst << " = -r" << inst.left; break;
        
        case OpCode::LOAD_CONST:  std::cout << "LOAD_CONST r" << inst.dst << " = " << vm.getConstants()[inst.left]; break;
                case OpCode::LOAD_VAR:    std::cout << "LOAD_VAR r" << inst.dst << " = mem[" << inst.left << "]"; break;
                case OpCode::LOAD_OUTER:  std::cout << "LOAD_OUTER r" << inst.dst << " hops=" << inst.left << " off=" << (int)(int8_t)inst.right; break;
                case OpCode::LOAD_STR:    std::cout << "LOAD_STR r" << inst.dst << " = \"" << vm.getStrings()[inst.left] << "\""; break;
        case OpCode::LOAD_NONE:   std::cout << "LOAD_NONE r" << inst.dst; break;
        case OpCode::LOAD:        std::cout << "LOAD r" << inst.dst << " = [fp" << ((int8_t)inst.right >= 0 ? "+" : "") << (int)(int8_t)inst.right << "]"; break;
        case OpCode::STORE:       std::cout << "STORE r" << inst.dst << " -> fp" << ((int8_t)inst.right >= 0 ? "+" : "") << (int)(int8_t)inst.right; break;
                case OpCode::STORE_VAR:   std::cout << "STORE_VAR r" << inst.right << " -> mem[" << inst.left << "]"; break;
                case OpCode::STORE_OUTER: std::cout << "STORE_OUTER r" << inst.dst << " hops=" << inst.left << " off=" << (int)(int8_t)inst.right; break;
        
        case OpCode::CMP_GT:      std::cout << "CMP_GT r" << inst.dst << " = r" << inst.left << " > r" << inst.right; break;
        case OpCode::CMP_LT:      std::cout << "CMP_LT r" << inst.dst << " = r" << inst.left << " < r" << inst.right; break;
        case OpCode::CMP_GET:     std::cout << "CMP_GET r" << inst.dst << " = r" << inst.left << " >= r" << inst.right; break;
        case OpCode::CMP_LET:     std::cout << "CMP_LET r" << inst.dst << " = r" << inst.left << " <= r" << inst.right; break;
        case OpCode::CMP_EQ:      std::cout << "CMP_EQ r" << inst.dst << " = r" << inst.left << " == r" << inst.right; break;
        case OpCode::CMP_NEQ:     std::cout << "CMP_NEQ r" << inst.dst << " = r" << inst.left << " != r" << inst.right; break;
        
        case OpCode::JMP:         std::cout << "JMP " << getAddress(inst); break;
        case OpCode::JZ:          std::cout << "JZ r" << inst.dst << " -> " << getAddress(inst); break;
        case OpCode::JNZ:         std::cout << "JNZ r" << inst.dst << " -> " << getAddress(inst); break;
        
        case OpCode::PRINT:       std::cout << "PRINT r" << inst.dst; break;
        case OpCode::PRINT_STR:   std::cout << "PRINT_STR \"" << vm.getStrings()[inst.dst] << "\""; break;
        
        case OpCode::LOGICAL_AND: std::cout << "LOGICAL_AND r" << inst.dst << " = r" << inst.left << " && r" << inst.right; break;
        case OpCode::LOGICAL_OR:  std::cout << "LOGICAL_OR r" << inst.dst << " = r" << inst.left << " || r" << inst.right; break;
        case OpCode::LOGICAL_NOT: std::cout << "LOGICAL_NOT r" << inst.dst << " = !r" << inst.left; break;
        
        case OpCode::CALL:        std::cout << "CALL r" << inst.dst << " -> " << getAddress(inst); break;
        case OpCode::RETURN:      std::cout << "RETURN r" << inst.dst; break;
        case OpCode::PUSH_ARG:    std::cout << "PUSH_ARG r" << inst.dst; break;
        case OpCode::LOAD_PARAM:  std::cout << "LOAD_PARAM r" << inst.dst << " = arg[" << inst.left << "]"; break;
        
        case OpCode::INPUT:       std::cout << "INPUT r" << inst.dst; break;
        case OpCode::LENGTH:      std::cout << "LENGTH r" << inst.dst << " = len(r" << inst.left << ")"; break;
        case OpCode::LOAD_STR_IDX: std::cout << "LOAD_STR_IDX r" << inst.dst << " = r" << inst.left << "[r" << inst.right << "]"; break;
        case OpCode::STORE_STR_IDX: std::cout << "STORE_STR_IDX r" << inst.left << "[r" << inst.right << "] = r" << inst.dst; break;
        case OpCode::ARRAY_NEW:    std::cout << "ARRAY_NEW r" << inst.dst << " = array(r" << inst.left << ")"; break;
        case OpCode::ARRAY_LIT:    std::cout << "ARRAY_LIT r" << inst.dst << " = []"; break;
        case OpCode::ARRAY_PUSH:   std::cout << "ARRAY_PUSH r" << inst.dst << " = push(r" << inst.left << ", r" << inst.right << ")"; break;
        case OpCode::ARRAY_POP:    std::cout << "ARRAY_POP r" << inst.dst << " = pop(r" << inst.left << ")"; break;
        case OpCode::ARRAY_INSERT: std::cout << "ARRAY_INSERT r" << inst.left << "[r" << inst.right << "] insert r" << inst.dst; break;
        case OpCode::ARRAY_REMOVE: std::cout << "ARRAY_REMOVE r" << inst.dst << " = remove(r" << inst.left << ", r" << inst.right << ")"; break;

        case OpCode::ARGC:            std::cout << "ARGC r" << inst.dst; break;
        case OpCode::COLLECT_VARARGS: std::cout << "COLLECT_VARARGS r" << inst.dst << " = args[" << inst.left << ":]"; break;
        
        case OpCode::SIN:         std::cout << "SIN r" << inst.dst << " = sin(r" << inst.left << ")"; break;
        case OpCode::COS:         std::cout << "COS r" << inst.dst << " = cos(r" << inst.left << ")"; break;
        case OpCode::TAN:         std::cout << "TAN r" << inst.dst << " = tan(r" << inst.left << ")"; break;
        case OpCode::ASIN:        std::cout << "ASIN r" << inst.dst << " = asin(r" << inst.left << ")"; break;
        case OpCode::ACOS:        std::cout << "ACOS r" << inst.dst << " = acos(r" << inst.left << ")"; break;
        case OpCode::ATAN:        std::cout << "ATAN r" << inst.dst << " = atan(r" << inst.left << ")"; break;
        case OpCode::ATAN2:       std::cout << "ATAN2 r" << inst.dst << " = atan2(r" << inst.left << ", r" << inst.right << ")"; break;
        case OpCode::SQRT:        std::cout << "SQRT r" << inst.dst << " = sqrt(r" << inst.left << ")"; break;
        case OpCode::EXP:         std::cout << "EXP r" << inst.dst << " = exp(r" << inst.left << ")"; break;
        case OpCode::LOG:         std::cout << "LOG r" << inst.dst << " = log(r" << inst.left << ")"; break;
        case OpCode::LOG10:       std::cout << "LOG10 r" << inst.dst << " = log10(r" << inst.left << ")"; break;
        case OpCode::LOG2:        std::cout << "LOG2 r" << inst.dst << " = log2(r" << inst.left << ")"; break;
        case OpCode::CEIL:        std::cout << "CEIL r" << inst.dst << " = ceil(r" << inst.left << ")"; break;
        case OpCode::FLOOR:       std::cout << "FLOOR r" << inst.dst << " = floor(r" << inst.left << ")"; break;
        case OpCode::ABS:         std::cout << "ABS r" << inst.dst << " = abs(r" << inst.left << ")"; break;
        case OpCode::ROUND:       std::cout << "ROUND r" << inst.dst << " = round(r" << inst.left << ")"; break;
        case OpCode::FMOD:        std::cout << "FMOD r" << inst.dst << " = fmod(r" << inst.left << ", r" << inst.right << ")"; break;
        case OpCode::CBRT:        std::cout << "CBRT r" << inst.dst << " = cbrt(r" << inst.left << ")"; break;
        case OpCode::MATH_POW:    std::cout << "POW r" << inst.dst << " = pow(r" << inst.left << ", r" << inst.right << ")"; break;
        case OpCode::LOG_AB:      std::cout << "LOG_AB r" << inst.dst << " = log(r" << inst.right << ") / log(r" << inst.left << ")"; break;
        
        case OpCode::CONST_PI:    std::cout << "CONST_PI r" << inst.dst; break;
        case OpCode::CONST_E:     std::cout << "CONST_E r" << inst.dst; break;
        case OpCode::CONST_INF:     std::cout << "CONST_INF r" << inst.dst; break;
        case OpCode::CONST_MAX:     std::cout << "CONST_MAX r" << inst.dst; break;
        
        case OpCode::ADDI:        std::cout << "ADDI r" << inst.dst << " = r" << inst.left << " + " << (int32_t)(int8_t)inst.right; break;
        case OpCode::ANDI:        std::cout << "ANDI r" << inst.dst << " = r" << inst.left << " & " << (int32_t)(int8_t)inst.right; break;
        case OpCode::ORI:         std::cout << "ORI r" << inst.dst << " = r" << inst.left << " | " << (int32_t)(int8_t)inst.right; break;
        case OpCode::XORI:        std::cout << "XORI r" << inst.dst << " = r" << inst.left << " ^ " << (int32_t)(int8_t)inst.right; break;
        case OpCode::SLLI:        std::cout << "SLLI r" << inst.dst << " = r" << inst.left << " << " << inst.right; break;
        case OpCode::SRLI:        std::cout << "SRLI r" << inst.dst << " = r" << inst.left << " >> " << inst.right; break;
        case OpCode::SRAI:        std::cout << "SRAI r" << inst.dst << " = r" << inst.left << " >>> " << inst.right; break;
        case OpCode::LW:          std::cout << "LW r" << inst.dst << " = mem[" << inst.left << "]"; break;
        case OpCode::SW:          std::cout << "SW mem[" << inst.left << "] = r" << inst.dst; break;
        
        case OpCode::TYPE:        std::cout << "TYPE r" << inst.dst << " = type(r" << inst.left << ")"; break;
        case OpCode::TO_NUMBER:   std::cout << "TO_NUMBER r" << inst.dst << " = number(r" << inst.left << ")"; break;
        case OpCode::TO_STRING:   std::cout << "TO_STRING r" << inst.dst << " = string(r" << inst.left << ")"; break;
        case OpCode::ORD:         std::cout << "ORD r" << inst.dst << " = ord(r" << inst.left << ")"; break;
        case OpCode::CHR:         std::cout << "CHR r" << inst.dst << " = chr(r" << inst.left << ")"; break;
        case OpCode::BIN:         std::cout << "BIN r" << inst.dst << " = bin(r" << inst.left << ")"; break;
        case OpCode::HEX:         std::cout << "HEX r" << inst.dst << " = hex(r" << inst.left << ")"; break;
        case OpCode::OCT:         std::cout << "OCT r" << inst.dst << " = oct(r" << inst.left << ")"; break;
        case OpCode::DEC:         std::cout << "DEC r" << inst.dst << " = dec(r" << inst.left << ")"; break;

        case OpCode::RANDOM: {
            if(inst.left == 0 && inst.right == 0) {
                std::cout << "RANDOM r" << inst.dst << " = random()";
            } else {
                std::cout << "RANDOM r" << inst.dst << " = random(r"
                          << inst.left << ", r" << inst.right << ")";
            }
        };
        break;

        default:                  std::cout << "OP(" << (int)op << ")"; break;
    }
    std::cout << std::endl;
}

void Debugger::visualize() const {
    std::cout << "\n[VM Bytecode Visualization]\n";
    std::cout << std::left
              << std::setw(7)  << "Addr"
              << std::setw(20) << "OpCode"
              << std::setw(6)  << "L"
              << std::setw(6)  << "R"
              << std::setw(6)  << "Dst"
              << "Value\n";
    std::cout << std::string(60, '-') << "\n";


    auto R = [](int n) -> std::string { return "r" + std::to_string(n); };

    auto IMM8 = [](uint8_t raw) -> std::string {
        return std::to_string(static_cast<int32_t>(static_cast<int8_t>(raw)));
    };


    const auto& prog      = vm.getProgram();
    const auto& constants = vm.getConstants();
    const auto& strings   = vm.getStrings();

    for (size_t i = 0; i < prog.size(); ++i) {
        const auto& inst = prog[i];
        OpCode op = static_cast<OpCode>(inst.op);

        std::string opStr  = "?";
        std::string lStr   = "-";
        std::string rStr   = "-";
        std::string dstStr = "-";
        std::string valStr = "";

        switch (op) {

        case OpCode::ADD:
            opStr  = "ADD "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SUB:
            opStr  = "SUB "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::MUL:
            opStr  = "MUL "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::DIV:
            opStr  = "DIV "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::MODULO:
            opStr  = "MOD "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::POW:
            opStr  = "POW "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::MATH_POW:
            opStr  = "MATH_POW " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::FLOOR_DIV:
            opStr  = "FLOOR_DIV " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::FRAC_DIV:
            opStr  = "FRAC_DIV " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::AND:
            opStr  = "AND "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::OR:
            opStr  = "OR "   + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::XOR:
            opStr  = "XOR "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SLL:
            opStr  = "SLL "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SRL:
            opStr  = "SRL "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SRA:
            opStr  = "SRA "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SLT:
            opStr  = "SLT "  + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SLTU:
            opStr  = "SLTU " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_GT:
            opStr  = "CMP_GT " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_LT:
            opStr  = "CMP_LT " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_GET:
            opStr  = "CMP_GET " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_LET:
            opStr  = "CMP_LET " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_EQ:
            opStr  = "CMP_EQ " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CMP_NEQ:
            opStr  = "CMP_NEQ " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOGICAL_AND:
            opStr  = "LOGICAL_AND " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOGICAL_OR:
            opStr  = "LOGICAL_OR " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ATAN2:
            opStr  = "ATAN2 " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::FMOD:
            opStr  = "FMOD " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOG_AB:
            opStr  = "LOG_AB " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::MOV:
            opStr  = "MOV "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::UNARY:
            opStr  = "NEG "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::NOT:
            opStr  = "NOT "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOGICAL_NOT:
            opStr  = "LOGICAL_NOT " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SIN:
            opStr  = "SIN "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::COS:
            opStr  = "COS "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::TAN:
            opStr  = "TAN "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ASIN:
            opStr  = "ASIN " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ACOS:
            opStr  = "ACOS " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ATAN:
            opStr  = "ATAN " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SQRT:
            opStr  = "SQRT " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::EXP:
            opStr  = "EXP "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOG:
            opStr  = "LOG "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOG10:
            opStr  = "LOG10 " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOG2:
            opStr  = "LOG2 " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CEIL:
            opStr  = "CEIL " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::FLOOR:
            opStr  = "FLOOR " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ABS:
            opStr  = "ABS "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ROUND:
            opStr  = "ROUND " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CBRT:
            opStr  = "CBRT " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::TYPE:
            opStr  = "TYPE " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::TO_NUMBER:
            opStr  = "TO_NUMBER " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::TO_STRING:
            opStr  = "TO_STRING " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ORD:
            opStr  = "ORD "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CHR:
            opStr  = "CHR "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::BIN:
            opStr  = "BIN "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::HEX:
            opStr  = "HEX "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::OCT:
            opStr  = "OCT "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::DEC:
            opStr  = "DEC "  + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LENGTH:
            opStr  = "LENGTH " + R(inst.left) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ADDI:
            opStr  = "ADDI " + R(inst.left) + " " + IMM8(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = IMM8(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ANDI:
            opStr  = "ANDI " + R(inst.left) + " " + IMM8(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = IMM8(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::ORI:
            opStr  = "ORI "  + R(inst.left) + " " + IMM8(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = IMM8(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::XORI:
            opStr  = "XORI " + R(inst.left) + " " + IMM8(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = IMM8(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SLLI:
            opStr  = "SLLI " + R(inst.left) + " " + std::to_string(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SRLI:
            opStr  = "SRLI " + R(inst.left) + " " + std::to_string(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SRAI:
            opStr  = "SRAI " + R(inst.left) + " " + std::to_string(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::LOAD_CONST: {
            double cv = (inst.left < (int)constants.size()) ? constants[inst.left] : 0.0;
            opStr  = "LOAD_CONST " + R(inst.dst);
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            valStr = std::to_string(cv);
            break;
        }
        case OpCode::LOAD_STR: {
            std::string sv = (inst.left < (int)strings.size()) ? strings[inst.left] : "?";
            opStr  = "LOAD_STR \"" + sv + "\" " + R(inst.dst);
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            valStr = "\"" + sv + "\"";
            break;
        }
        case OpCode::LOAD_VAR:
            opStr  = "LOAD_VAR mem[" + std::to_string(inst.left) + "] " + R(inst.dst);
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::STORE_VAR:
            opStr  = "STORE_VAR " + R(inst.right) + " mem[" + std::to_string(inst.left) + "]";
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            break;
        case OpCode::LOAD_NONE:
            opStr  = "LOAD_NONE " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOAD_OUTER: {
            int8_t off = static_cast<int8_t>(inst.right);
            opStr  = "LOAD_OUTER hops=" + std::to_string(inst.left)
                   + " off=" + std::to_string(static_cast<int>(off))
                   + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(static_cast<int>(off)); dstStr = std::to_string(inst.dst);
            break;
        }
        case OpCode::STORE_OUTER: {
            int8_t off = static_cast<int8_t>(inst.right);
            opStr  = "STORE_OUTER " + R(inst.dst)
                   + " hops=" + std::to_string(inst.left)
                   + " off=" + std::to_string(static_cast<int>(off));
            lStr = std::to_string(inst.left); rStr = std::to_string(static_cast<int>(off)); dstStr = std::to_string(inst.dst);
            break;
        }
        case OpCode::LOAD: {
            int8_t off = static_cast<int8_t>(inst.right);
            std::string sign = (off >= 0) ? "+" : "";
            opStr  = "LOAD [fp" + sign + std::to_string(static_cast<int>(off)) + "] " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(static_cast<int>(off)); dstStr = std::to_string(inst.dst);
            break;
        }
        case OpCode::STORE: {
            int8_t off = static_cast<int8_t>(inst.right);
            std::string sign = (off >= 0) ? "+" : "";
            opStr  = "STORE " + R(inst.dst) + " -> fp" + sign + std::to_string(static_cast<int>(off));
            lStr = std::to_string(inst.left); rStr = std::to_string(static_cast<int>(off)); dstStr = std::to_string(inst.dst);
            break;
        }
        case OpCode::LW:
            opStr  = "LW mem[" + std::to_string(inst.left) + "] " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::SW:
            opStr  = "SW " + R(inst.dst) + " mem[" + std::to_string(inst.left) + "]";
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::LOAD_STR_IDX:
            opStr  = "LOAD_STR_IDX " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        case OpCode::STORE_STR_IDX:
            opStr  = "STORE_STR_IDX " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_NEW:
            opStr  = "ARRAY_NEW " + R(inst.dst) + " = array(" + R(inst.left) + ")";
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_LIT:
            opStr  = "ARRAY_LIT " + R(inst.dst) + " = []";
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_PUSH:
            opStr  = "ARRAY_PUSH " + R(inst.dst) + " = push(" + R(inst.left) + ", " + R(inst.right) + ")";
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_POP:
            opStr  = "ARRAY_POP " + R(inst.dst) + " = pop(" + R(inst.left) + ")";
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_INSERT:
            opStr  = "ARRAY_INSERT " + R(inst.left) + "[" + R(inst.right) + "] = " + R(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARRAY_REMOVE:
            opStr  = "ARRAY_REMOVE " + R(inst.dst) + " = remove(" + R(inst.left) + ", " + R(inst.right) + ")";
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::ARGC:
            opStr  = "ARGC " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::COLLECT_VARARGS:
            opStr  = "COLLECT_VARARGS " + R(inst.dst) + " = args[" + std::to_string(inst.left) + ":]";
            lStr   = std::to_string(inst.left);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::JMP: {
            size_t addr = static_cast<size_t>(getAddress(inst));
            opStr  = "JMP " + std::to_string(addr);
            dstStr = std::to_string(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            break;
        }
        case OpCode::JZ: {
            size_t addr = static_cast<size_t>(getAddress(inst));
            opStr  = "JZ " + R(inst.dst) + " " + std::to_string(addr);
            dstStr = std::to_string(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            break;
        }
        case OpCode::JNZ: {
            size_t addr = static_cast<size_t>(getAddress(inst));
            opStr  = "JNZ " + R(inst.dst) + " " + std::to_string(addr);
            dstStr = std::to_string(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            break;
        }
        case OpCode::CALL: {
            size_t addr = static_cast<size_t>(getAddress(inst));
            opStr  = "CALL " + std::to_string(addr) + " " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            lStr   = std::to_string(inst.left);
            rStr   = std::to_string(inst.right);
            break;
        }
        case OpCode::RETURN:
            opStr  = "RETURN " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::PUSH_ARG:
            opStr  = "PUSH_ARG " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::LOAD_PARAM:
            opStr  = "LOAD_PARAM arg[" + std::to_string(inst.left) + "] " + R(inst.dst);
            lStr = std::to_string(inst.left); dstStr = std::to_string(inst.dst);
            break;

        case OpCode::PRINT:
            opStr  = "PRINT " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::PRINT_STR: {
            std::string sv = (inst.dst < (int)strings.size()) ? strings[inst.dst] : "?";
            std::string escaped;
            escaped.reserve(sv.size());
            for (char c : sv) {
                switch (c) {
                    case '\n': escaped += "\\n";  break;
                    case '\r': escaped += "\\r";  break;
                    case '\t': escaped += "\\t";  break;
                    case '\\': escaped += "\\\\"; break;
                    case '"':  escaped += "\\\""; break;
                    default:   escaped += c;      break;
                }
            }
            opStr  = "PRINT_STR \"" + escaped + "\"";
            dstStr = std::to_string(inst.dst);
            valStr = "\"" + escaped + "\"";
        }
        break;
        case OpCode::INPUT:
            opStr  = "INPUT " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::CONST_PI:
            opStr  = "CONST_PI " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CONST_E:
            opStr  = "CONST_E " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CONST_INF:
            opStr  = "CONST_INF " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;
        case OpCode::CONST_MAX:
            opStr  = "CONST_MAX " + R(inst.dst);
            dstStr = std::to_string(inst.dst);
            break;

        case OpCode::RANDOM:
            if (inst.left == 0 && inst.right == 0) {
                opStr = "RANDOM " + R(inst.dst);
            } else {
                opStr = "RANDOM " + R(inst.left) + " " + R(inst.right) + " " + R(inst.dst);
                lStr = std::to_string(inst.left); rStr = std::to_string(inst.right);
            }
            dstStr = std::to_string(inst.dst);
            break;

        default:
            opStr  = "UNKNOWN(op=" + std::to_string(static_cast<int>(op)) + ")";
            lStr = std::to_string(inst.left); rStr = std::to_string(inst.right); dstStr = std::to_string(inst.dst);
            break;
        }

        std::cout << std::left
                  << "[" << std::setw(3) << i << "]  "
                  << std::setw(20) << opStr
                  << std::setw(6)  << lStr
                  << std::setw(6)  << rStr
                  << std::setw(6)  << dstStr
                  << valStr
                  << "\n";
    }
}