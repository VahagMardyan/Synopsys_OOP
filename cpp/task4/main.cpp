#include "parser.h"

double calculate(const std::string& expression, SymbolTable& symTable) {
    std::istringstream stream(expression);

    Lexer lexer(stream);
    Tokenizer tokenizer(lexer);
    Parser parser(tokenizer, symTable);
    auto root = parser.parse();
    if(!root) {
        throw std::runtime_error("Parsing error or empty AST");
    }
    return root -> evaluate();
}

int main() {
    SymbolTable st;
    size_t x_addr = st.getAddress("x");
    size_t y_addr = st.getAddress("y");
    size_t z_addr = st.getAddress("z");
    // st.setValueByAddress(x_addr, 5.05);
    // st.setVariable("delta_var", 5e-13);
    
    std::string expr = "x*(y+54) - z/12 + (5+4.25)*(-0.8)";
    std::istringstream stream(expr);
    Lexer lexer(stream);
    Tokenizer tokenizer(lexer);
    Parser parser(tokenizer, st);
    auto root = parser.parse();

    // if(root) {
    //     std::cout<<"---AST Visualization---"<<std::endl;
    //     root -> print();
    //     std::cout<<"-----------------------"<<std::endl;
    //     std::cout<<"Result: "<<root -> evaluate() << std::endl;
    // }
    // double res = calculate(expr, st);
    // std::cout<<"Result: "<<res<<std::endl;

    if(root) {
        for(double i = 0, j=1, k=2; i<10, j<6, k<8; i += 1.0, j += 1.0, k+=1.0) {
            st.setValueByAddress(x_addr, i);
            st.setValueByAddress(y_addr, j);
            st.setValueByAddress(z_addr, k);
            double result = root -> evaluate();
            std::cout<<"(x,y,z): "<<i<<" "<<j<<" "<<k<<" Res: "<<result<<std::endl;
        }
    }
    return 0;
}