#include "parser.h"
#include <iomanip>

void disassemble(const std::vector<Instruction>& program);
double runVM(const std::vector<Instruction>& program, const SymbolTable& symTable, int tempSize, int finalSize);