#pragma once

#include <vector>
#include <unordered_map>
#include <stack>
#include <stdexcept>
#include <memory>
#include <string>
#include <cctype>

class SymbolTable {
    private:
        std::unordered_map<std::string, size_t> nameToIndex;
        std::vector<double> memory;

    public:
        size_t getAddress(const std::string&);
        void setValueByAddress(size_t, double);
        double getValueByAddress(size_t) const;
        void setVariable(const std::string&, double);
};

