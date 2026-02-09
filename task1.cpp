#include <iostream>
#include <stack>
#include <string>
#include <sstream>

int precendence(char op) {
    if(op == '+' || op == '-') return 1;
    if(op == '*' || op == '/') return 2;
    return 0;
}

std::string infixToPostfix(std::string s) {
    std::stack<char> st;
    std::string result;

    for(int i=0;i<s.length();i++) {
        char c = s[i];
        
        if(isspace(c)) continue;
        
        if(c == '-' && (i == 0 || s[i-1] == '(')) {
            result += '-';
            continue;
        }

        if(isdigit(c)) {
            result += c;
            if(i+1 >= s.length() || !isdigit(s[i+1])) {
                result += ' ';
            }
        } else if(c == '(') {
            st.push(c);
        } else if(c == ')') {
            while(!st.empty() &&  st.top() != '(') {
                result += st.top();
                result += ' ';
                st.pop();
            }
            if(!st.empty()) {
                st.pop();
            }
        } else {
            while(!st.empty() && precendence(st.top()) >= precendence(c)) {
                result += st.top();
                result += ' ';
                st.pop();
            }
            st.push(c);
        }
    }
    while(!st.empty()) {
        result += st.top();
        result += ' ';
        st.pop();
    }
    return result;
}

int simplified_eval(std::string postfix) {
    std::stack<int> st;
    std::string token;
    std::stringstream ss(postfix);
    while(ss >> token) {
        if(isdigit(token[0]) || (token.length() > 1 && token[0] == '-')) {
            st.push(std::stoi(token));
        } else {
            int right = st.top(); st.pop();
            int left = st.top(); st.pop();

            if(token == "+") {
                st.push(left + right);
            } else if(token == "-") {
                st.push(left - right);
            } else if(token == "*") {
                st.push(left * right);
            } else if(token == "/") {
                st.push(left / right);
            }
        }
    }
    return st.top();
}

int main() {
    std::string expr;
    std::cout<<"Input Math Expression: ";
    std::getline(std::cin, expr);
    int result = simplified_eval(infixToPostfix(expr));
    std::cout << "Result: " << result<<std::endl;
    return 0;
}