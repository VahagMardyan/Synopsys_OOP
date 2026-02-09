#include <regex>
#include <stack>

bool isValidExpression(std::string expr) {
    std::regex pattern(R"(^[\d\s\+\-\*\/\(\)]+$)");
    if(!std::regex_match(expr, pattern)) {
        return false;
    }

    std::regex doubleOp(R"([\+\*\/]{2,})");
    if(std::regex_match(expr, doubleOp)) {
        return false;
    }

    std::stack<char> st;
    for(char c : expr) {
        if( c == '(') {
            st.push(c);
        } else if(c == ')') {
            if(st.empty()) {
                return false;
            }
            st.pop();
        }
    }
    if(!st.empty()) {
        return false;
    }

    size_t first = expr.find_first_not_of(" ");
    if(first != std::string::npos) {
        char firstChar = expr[first];
        if(firstChar == '*' || firstChar == '/' || firstChar == ')') {
            return false;
        }
    }

    size_t last = expr.find_last_not_of(" ");
    if(last != std::string::npos) {
        char lastChar = expr[last];
        if(precendence(lastChar) > 0 || lastChar == ')') { // task1.cpp
            return false;
        }
    }

    return true;
}