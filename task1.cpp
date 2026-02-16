#include <iostream>
#include <stack>
#include <string>
#include <sstream>
#include <map>

int precendence(char op) {
    if(op == '+' || op == '-') return 1;
    if(op == '*' || op == '/') return 2;
    return 0;
}

std::string toPostfix(std::string s) {
    std::stack<char> st;
    std::string result;

    for(int i=0;i<s.length();i++) {
        char c = s[i];
        
        if(isspace(c)) continue;
        
        if(c == '-' && (i == 0 || s[i-1] == '(')) {
            result += '-';
            continue;
        }

        if(isalnum(c) || c == '.') {
            result += c;
            if(i+1 >= s.length() || (!isalnum(s[i+1]) && s[i+1] != '.')) {
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

double evaluate(std::string postfix, std::map<std::string, double>& variables) {
    std::stack<double> st;
    std::string token;
    std::stringstream ss(postfix);
    while(ss >> token) {
        if(isdigit(token[0]) || (token.length() > 1 && isdigit(token[1]))) {
            st.push(std::stod(token));
        } else if(isalpha(token[0]) || (token[0] == '-' && token.length() > 1 && isalpha(token[1]))) {
            std::string varName = token;
            double sign = 1.0;
            if(token[0] == '-') {
                sign = -1.0;
                varName = token.substr(1);
            }
            if(variables.count(varName) == 0) {
                std::cout<<"Enter value for " << token << ": "; 
                std::cin>>variables[varName];
            }
            st.push(sign * variables[varName]);
        } else {
            double right = st.top(); st.pop();
            double left = st.top(); st.pop();

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
    std::map<std::string, double> vars;
    // vars["x"] = 5;
    // vars["y"] = 7;
    std::string expr = "(a+b)*5+b";
    std::cout<<"Expression is: "<<expr<<std::endl;
    int cnt = 0;
    for(double a=0; a<=10; a+=0.5) {
        std::map<std::string, double> x;
        x["a"] = a;
        for(double b = 0; b<=10; b+=0.5) {
            x["b"] = b;
            double res = evaluate(toPostfix(expr),x);
            std::cout<<"(a;b): "<<a<<":"<<b<<" "<< "out: " << res <<std::endl;
            cnt++;
        }
    }
    // std::cout<<"Input Math Expression: ";
    // std::getline(std::cin, expr);
    // std::string postfix = toPostfix(expr);
    // double result = evaluate(postfix, vars);
    // std::cout << "Postfix: " << postfix << std::endl;
    // std::cout << "Result: " << result << std::endl;
    std::cout<<cnt<<std::endl;
    return 0;
}