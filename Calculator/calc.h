#include "parser.h"
#include <iomanip>

class Calculate {
    private:
        std::vector<Instruction> program;
        int tempSize = 0;
        int finalIdx = 0;
        bool debug_mode = false;
        void visualize();
    public:
        Calculate(bool dm = false) : debug_mode(dm) {}
        double execute(const SymbolTable& symTable);
        void compile(const std::string& expr, SymbolTable& symTable);
};