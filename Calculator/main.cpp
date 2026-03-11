#include "calc.h"
#include <chrono>

int main() {
    SymbolTable st;
    size_t x_addr = st.getAddress("x");
    size_t y_addr = st.getAddress("y");
    size_t z_addr = st.getAddress("z");
    // st.setVariable("x", 0.0);
    // st.setVariable("y", 0.0);
    // st.setVariable("z", 0.0);

    std::vector<Instruction> program;
    int tempSize = 0;
    int finalIdx = 0;

    {
        std::string expr = "(x*x + y*y + z*z) * (-0.5 + x*y / 100)";
        expr = "(x+5*(y+68)-0.128)/(z+6.5)";
        std::cout<<"Compiling expression: "<<expr<<std::endl;

        std::istringstream stream(expr);
        Lexer lexer(stream);
        Tokenizer tokenizer(lexer);
        Parser parser(tokenizer, st);
        auto root = parser.parse();
        // root -> print();
        if(!root) {
            std::cerr << "Compilation failed!"<<std::endl;
            return 1;
        }
        finalIdx = root -> compile(program, tempSize);
        // disassemble(program);
    }
    // st.setValueByAddress(x_addr, 2);
    // st.setValueByAddress(y_addr, 3);
    // st.setValueByAddress(z_addr, 1);
    // double result = runVM(program, st, tempSize, finalIdx);
    // std::cout<<result<<std::endl;
    const int iterations = 1000000;
    std::cout<<"\nStarting VM execution (1.000.000 iterations)..."<<std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    for(int i=0; i < iterations; ++i) {
        double val = static_cast<double>(i);
        st.setValueByAddress(x_addr, val * 0.01);
        st.setValueByAddress(y_addr, val * 0.02);
        st.setValueByAddress(z_addr, val * 0.03);
        double result = runVM(program, st, tempSize, finalIdx);
        if (i % 200000 == 0) {
            std::cout << "Step " << i << ": x=" << val*0.01 << ", y=" << val*0.02 << ", z=" << val*0.03 
                      << " => Result: " << result << std::endl;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "\nFinished "<< iterations <<" calculations in: " << diff.count() << " seconds." << std::endl;
    return 0;
}