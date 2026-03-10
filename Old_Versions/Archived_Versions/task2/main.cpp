#include "task2.h"
#include <chrono>

int main() {
    std::string expr = "((var1 * -2.5) + (42 / (var2 - 3.5))) * (var1 - (var3 / 2)) + (var4 * (var1 + var2)) - -0.123";
    std::map<std::string, double> vars = {
        {
            "var1", 2.45
        },
        {
            "var2", -0.0025
        },
        {
            "var3", 5000
        },
        {
            "var4", 2015.1
        }
    };
    auto start = std::chrono::high_resolution_clock::now();
    Parser parser(expr);
    auto root = parser.parseExpression();
    double result = root -> eval(vars);
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<"Time: "<<duration.count()<<"ns | Result: "<<result<<std::endl;
    return 0;
}