#pragma once
#include <iostream>
#include <stack>
#include <sstream>
#include <string>
#include <map>

int precendence(char);
bool isOperator(char);
std::string toPostfix(std::string);
double evaluate(std::string, std::map<std::string, double>&);
