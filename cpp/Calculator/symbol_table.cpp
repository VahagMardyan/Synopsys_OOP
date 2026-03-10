#include "symbol_table.h"

size_t SymbolTable::getAddress(const std::string& name) {
    auto it = nameToIndex.find(name);
        if (it != nameToIndex.end()) {
            return it->second;
        }
        size_t newAddress = memory.size();
        nameToIndex[name] = newAddress;
        memory.push_back(0.0);
        return newAddress;
}

void SymbolTable::setValueByAddress(size_t address, double value) {
    if(address < memory.size()) {
        memory[address] = value;
    }
}

double SymbolTable::getValueByAddress(size_t address) const {
    if(address < memory.size()) {
        return memory[address];
    }
    throw std::runtime_error("Invalid address");
}

void SymbolTable::setVariable(const std::string& name, double value) {
    size_t addr = getAddress(name);
    memory[addr] = value;
}