#include "task1.h"

int precendence(char op) {
    if(op == '+' || op == '-') return 1;
    if(op == '*' || op == '/') return 2;
    return 0;
}

std::string toPostfix(std::string s) {
    std::stack<char> st;
    std::string result;
    bool lastWasOp = true;

    for(int i = 0; i < s.length(); i++) {
        char c = s[i];
        if(isspace(c)) continue;

        if(c == '-' && lastWasOp) {
            result += "0 ";
            st.push('-');
            lastWasOp = false;
            continue;
        }

        if(isalnum(c) || c == '.') {
            while(i < s.length() && (isalnum(s[i]) || s[i] == '.')) {
                result += s[i++];
            }
            result += ' ';
            i--; 
            lastWasOp = false;
        } else if(c == '(') {
            st.push(c);
            lastWasOp = true;
        } else if(c == ')') {
            while(!st.empty() && st.top() != '(') {
                result += st.top();
                result += " ";
                st.pop();
            }
            if(!st.empty()) st.pop();
            lastWasOp = false;
        } else {
            while(!st.empty() && precendence(st.top()) >= precendence(c)) {
                result += st.top();
                result += " ";
                st.pop();
            }
            st.push(c);
            lastWasOp = true;
        }
    }
    while(!st.empty()) {
        if(st.top() != '(') {
            result += st.top();
            result += " ";
        }
        st.pop();
    }
    return result;
}

double evaluate(std::string postfix, std::map<std::string, double>& variables) {
    std::stack<double> st;
    std::stringstream ss(postfix);
    std::string token;

    while(ss >> token) {
        if(isdigit(token[0]) || (token.length() > 1 && isdigit(token[1]))) {
            st.push(std::stod(token));
        } else if(isalpha(token[0])) {
            if(variables.find(token) != variables.end()) {
                st.push(variables.at(token));
            } else {
                st.push(0.0);
            }
        } else if(token.length() == 1 && (token[0] == '+' || token[0] == '-' || token[0] == '*' || token[0] == '/')) {
            if(st.size() < 2) continue;
            double right = st.top(); st.pop();
            double left = st.top(); st.pop();
            if(token == "+") st.push(left + right);
            else if(token == "-") st.push(left - right);
            else if(token == "*") st.push(left * right);
            else if(token == "/") st.push(left / right);
        }
    }
    return st.empty() ? 0 : st.top();
}