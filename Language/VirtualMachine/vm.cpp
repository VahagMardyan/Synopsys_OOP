#include "vm.h"
#include "debugger.h"
#include <cmath>
#include <limits>
#include <iomanip>
#include <random>

namespace {
    int32_t toInt32(const Value& v) {
        return static_cast<int32_t>(asNumber(v));
    }

    double fromInt32(int32_t v) {
        return static_cast<double>(v);
    }

    Value resolveOuterFrameFp(int hops, const std::vector<CallFrame>& frames) {
        if (hops < 1 || static_cast<size_t>(hops) > frames.size()) {
            throw std::runtime_error("Invalid enclosing frame access (call stack depth)");
        }
        size_t idx = frames.size() - static_cast<size_t>(hops);
        return frames[idx].callerFp;
    }

    int32_t stringIndexFromValue(const Value& idxVal) {
        if (!isNumber(idxVal)) {
            throw std::runtime_error("Index must be a number");
        }
        double d = asNumber(idxVal);
        if (d != std::floor(d)) {
            throw std::runtime_error("Index must be an integer");
        }
        return static_cast<int32_t>(d);
    }

    void checkStringBounds(const std::string& s, int32_t idx) {
        if (idx < 0 || static_cast<size_t>(idx) >= s.size()) {
            throw std::runtime_error("String index out of bounds");
        }
    }

    void checkArrayBounds(const std::vector<Value>& items, int32_t idx) {
        if (idx < 0 || static_cast<size_t>(idx) >= items.size()) {
            throw std::runtime_error("Array index out of bounds");
        }
    }

    char singleCharFromValue(const Value& v) {
        if (!isString(v)) {
            throw std::runtime_error("String subscript assignment requires a string value");
        }
        const std::string& s = asString(v);
        if (s.size() != 1) {
            throw std::runtime_error("String subscript assignment requires a single character");
        }
        return s[0];
    }
}

// Shared RNG - seeded once at startup, reused across all RANDOM instructions.
static std::mt19937_64 s_rng{std::random_device{}()};

VirtualMachine::VirtualMachine(bool debugMode) : debug_mode(debugMode) {
    if (debug_mode) {
        debugger = std::make_unique<Debugger>(*this);
    }
}

VirtualMachine::~VirtualMachine() = default;

void VirtualMachine::loadFromFile(const std::string& byteCodePath) {
    ByteCode bc = readByteCodeFromFile(byteCodePath);
    loadByteCode(bc);
}

void VirtualMachine::load(const ByteCode& bc) {
    loadByteCode(bc);
}

void VirtualMachine::loadByteCode(const ByteCode& bc) {
    current_program  = bc.instructions;
    current_constants = bc.constants;
    current_strings = bc.strings;
    current_lineNumbers = bc.lineNumbers;

    vmGlobalSlotCount = bc.globalSlotCount;
    vmGlobalNames = bc.globalNamesBySlot;
    globalDefined.assign(vmGlobalSlotCount, false);

    int maxReg = 0;
    for(const auto& inst : current_program)
        maxReg = std::max({maxReg, (int)inst.left, (int)inst.right, (int)inst.dst});
    const int kRv32RegCount = 32;
    const int kSpReg = 2; // x2 (sp)
    const int kFpReg = 8; // x8 (s0/fp)
    registers.assign(std::max(kRv32RegCount, maxReg + 1), 0.0);
    registers[kSpReg] = 10000.0;
    registers[kFpReg] = 10000.0;
    memory.resize(65536);
}

size_t VirtualMachine::executeSingleInstruction() {
    if (pc >= current_program.size()) return pc;
    const auto& inst = current_program[pc];
    OpCode op = static_cast<OpCode>(inst.op);
    lastDestReg = inst.dst;
            bool jumped = false;
            switch(op) {
                case OpCode::LOAD_CONST: registers[inst.dst] = current_constants[inst.left]; break;
                case OpCode::LOAD_VAR: {
                    size_t addr = inst.left;
                    if (addr < globalDefined.size() && !globalDefined[addr]) {
                        std::string label = (addr < vmGlobalNames.size() && !vmGlobalNames[addr].empty())
                            ? vmGlobalNames[addr] : ("slot " + std::to_string(addr));
                        throw std::runtime_error("Uninitialized global variable: " + label);
                    }
                    registers[inst.dst] = memory[addr];
                    break;
                }
                case OpCode::LOAD_OUTER: {
                    int hops = static_cast<int>(inst.left);
                    int8_t off = static_cast<int8_t>(inst.right);
                    Value outerFp = resolveOuterFrameFp(hops, callStack);
                    int32_t addr = (int32_t)asNumber(outerFp) + off;
                    if (addr < 0 || addr >= (int32_t)memory.size()) {
                        throw std::runtime_error("LOAD_OUTER out of range");
                    }
                    registers[inst.dst] = memory[addr];
                    break;
                }
                case OpCode::LOAD_STR: registers[inst.dst] = current_strings[inst.left]; break;
                case OpCode::LOAD_NONE: registers[inst.dst] = std::monostate{}; break;
                case OpCode::STORE_VAR: {
                    size_t addr = inst.left;
                    memory[addr] = registers[inst.right];
                    if (addr < globalDefined.size()) {
                        globalDefined[addr] = true;
                    }
                    break;
                }
                case OpCode::STORE_OUTER: {
                    int hops = static_cast<int>(inst.left);
                    int8_t off = static_cast<int8_t>(inst.right);
                    Value outerFp = resolveOuterFrameFp(hops, callStack);
                    int32_t addr = (int32_t)asNumber(outerFp) + off;
                    if (addr < 0 || addr >= (int32_t)memory.size()) {
                        throw std::runtime_error("STORE_OUTER out of range");
                    }
                    memory[addr] = registers[inst.dst];
                    break;
                }
                case OpCode::MOV: registers[inst.dst] = registers[inst.left]; break;
                
                case OpCode::ADD:  {
                    if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                        throw std::runtime_error("Cannot add with None");
                    }
                    if(isArray(registers[inst.left]) && isArray(registers[inst.right])) {
                        auto result = std::make_shared<ArrayObj>();
                        const auto& l = asArray(registers[inst.left])->items;
                        const auto& r = asArray(registers[inst.right])->items;
                        result->items.reserve(l.size() + r.size());
                        result->items.insert(result->items.end(), l.begin(), l.end());
                        result->items.insert(result->items.end(), r.begin(), r.end());
                        registers[inst.dst] = result;
                    } else if(isArray(registers[inst.left]) || isArray(registers[inst.right])) {
                        throw std::runtime_error("Cannot add an array and a non-array (use array_push to append a single element)");
                    } else if(isString(registers[inst.left]) || isString(registers[inst.right])) {
                        registers[inst.dst] = valueToString(registers[inst.left]) + valueToString(registers[inst.right]);
                    } else {
                        registers[inst.dst] = asNumber(registers[inst.left]) + asNumber(registers[inst.right]);
                    }
                }
            break;
            case OpCode::SUB: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot sub with None");
                }
                registers[inst.dst] = asNumber(registers[inst.left]) - asNumber(registers[inst.right]); 
            }
            break;
            case OpCode::MUL: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot mul with None");
                }
                if(isString(registers[inst.left]) && isNumber(registers[inst.right])) {
                    const std::string& str = asString(registers[inst.left]);
                    double count = asNumber(registers[inst.right]);
                    int intCount = static_cast<int>(count);
                    if(intCount < 0) {
                        throw std::runtime_error("Cannot multiply string by negative number");
                    }
                    std::string result;
                    for(int i=0; i<intCount; ++i) {
                        result += str;
                    }
                    registers[inst.dst] = result;
                } else if(isNumber(registers[inst.left]) && isString(registers[inst.right])) {
                    const std::string& str = asString(registers[inst.right]);
                    double count = asNumber(registers[inst.left]);
                    int intCount = static_cast<int>(count);
                    if(intCount < 0) {
                        throw std::runtime_error("Cannot multiply string by negative number");
                    }
                    std::string result;
                    for(int i = 0; i < intCount; ++i) {
                        result += str;
                    }
                    registers[inst.dst] = result;
                } else if(isArray(registers[inst.left]) && isNumber(registers[inst.right])) {
                    const auto& items = asArray(registers[inst.left])->items;
                    double count = asNumber(registers[inst.right]);
                    int intCount = static_cast<int>(count);
                    if(intCount < 0) {
                        throw std::runtime_error("Cannot multiply array by negative number");
                    }
                    auto result = std::make_shared<ArrayObj>();
                    result->items.reserve(items.size() * static_cast<size_t>(intCount));
                    for(int i = 0; i < intCount; ++i) {
                        result->items.insert(result->items.end(), items.begin(), items.end());
                    }
                    registers[inst.dst] = result;
                } else if(isNumber(registers[inst.left]) && isArray(registers[inst.right])) {
                    const auto& items = asArray(registers[inst.right])->items;
                    double count = asNumber(registers[inst.left]);
                    int intCount = static_cast<int>(count);
                    if(intCount < 0) {
                        throw std::runtime_error("Cannot multiply array by negative number");
                    }
                    auto result = std::make_shared<ArrayObj>();
                    result->items.reserve(items.size() * static_cast<size_t>(intCount));
                    for(int i = 0; i < intCount; ++i) {
                        result->items.insert(result->items.end(), items.begin(), items.end());
                    }
                    registers[inst.dst] = result;
                } else if(isArray(registers[inst.left]) || isArray(registers[inst.right])) {
                    throw std::runtime_error("Arrays can only be multiplied by a number (repetition)");
                } else {
                    registers[inst.dst] = asNumber(registers[inst.left]) * asNumber(registers[inst.right]);
                }
            }   
            break;
            case OpCode::POW: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot pow with None");
                }
                registers[inst.dst] = std::pow(asNumber(registers[inst.left]), asNumber(registers[inst.right]));
            }  
            break;
            case OpCode::DIV: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot div with None");
                }
                if(asNumber(registers[inst.right]) == 0) throw std::runtime_error("Division by zero");
                registers[inst.dst] = asNumber(registers[inst.left]) / asNumber(registers[inst.right]); 
            }
            break;
            case OpCode::FLOOR_DIV: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot floor_div with None");
                }
                if(asNumber(registers[inst.right]) == 0) throw std::runtime_error("Division by zero");
                registers[inst.dst] = std::floor(asNumber(registers[inst.left]) / asNumber(registers[inst.right]));
            }
            break;
            case OpCode::FRAC_DIV: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot frac_div with None");
                }
                double l = asNumber(registers[inst.left]);
                double r = asNumber(registers[inst.right]);
                if(r == 0) throw std::runtime_error("Division by zero");
                registers[inst.dst] = l/r - std::floor(l/r);
            }
            break;
            case OpCode::CONST_PI: {
                registers[inst.dst] = 3.14159265358979323846;
            }
            break;
            case OpCode::CONST_E: {
                registers[inst.dst] = 2.718281828459045;
            }
            break;
            case OpCode::CONST_INF: {
                registers[inst.dst] = std::numeric_limits<double>::infinity();
            }
            break;
            case OpCode::CONST_MAX: {
                registers[inst.dst] = std::numeric_limits<double>::max();
            }
            break;
            case OpCode::UNARY:  
                if (isNone(registers[inst.left]))
                    throw std::runtime_error("Cannot apply unary minus to 'none'");
                registers[inst.dst] = -asNumber(registers[inst.left]); 
            break;
    
            case OpCode::MODULO: 
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use modulo with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) % toInt32(registers[inst.right])); 
            break;
    
            case OpCode::NOT: {
                if(isNone(registers[inst.left])) {
                    throw std::runtime_error("Cannot apply bitwise NOT to 'none'");
                }
                int32_t val = toInt32(registers[inst.left]);
                registers[inst.dst] = fromInt32(~val);
            }
            break;
    
            case OpCode::AND:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use bitwise AND with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) & toInt32(registers[inst.right])); 
            break;
    
            case OpCode::OR:     
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use bitwise OR with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) | toInt32(registers[inst.right])); 
            break;
    
            case OpCode::XOR:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use bitwise XOR with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) ^ toInt32(registers[inst.right])); 
            break;
    
            case OpCode::SLL:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use left shift with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) << (toInt32(registers[inst.right]) & 0x1F)); 
            break;
    
            case OpCode::SRL:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use right shift (logical) with 'none'");
                registers[inst.dst] = fromInt32((uint32_t)toInt32(registers[inst.left]) >> (toInt32(registers[inst.right]) & 0x1F)); 
            break;
    
            case OpCode::SRA:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot use right shift (arithmetic) with 'none'");
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) >> (toInt32(registers[inst.right]) & 0x1F)); 
            break;
    
            case OpCode::SLT:    
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot compare 'none' with <");
                registers[inst.dst] = toInt32(registers[inst.left]) < toInt32(registers[inst.right]) ? 1.0 : 0.0; 
            break;
    
            case OpCode::SLTU:   
                if (isNone(registers[inst.left]) || isNone(registers[inst.right]))
                    throw std::runtime_error("Cannot compare 'none' with unsigned <");
                registers[inst.dst] = (uint32_t)toInt32(registers[inst.left]) < (uint32_t)toInt32(registers[inst.right]) ? 1.0 : 0.0; 
            break;
    
            case OpCode::CMP_LT: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare None with <");
                }
                if(!isNumber(registers[inst.left]) || !isNumber(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare non-number values with <");
                }
                registers[inst.dst] = asNumber(registers[inst.left]) < asNumber(registers[inst.right]) ? 1.0 : 0.0;
            }
            break;
            case OpCode::CMP_GT: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare None with >");
                }
                if(!isNumber(registers[inst.left]) || !isNumber(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare non-number values with >");
                }
                registers[inst.dst] = asNumber(registers[inst.left]) >  asNumber(registers[inst.right]) ? 1.0 : 0.0; 
            }
            break;
            case OpCode::CMP_GET: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare None with >=");
                }
                if(!isNumber(registers[inst.left]) || !isNumber(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare non-number values with >=");
                }
                registers[inst.dst] = asNumber(registers[inst.left]) >= asNumber(registers[inst.right]) ? 1.0 : 0.0;
            }
            break;
            case OpCode::CMP_LET: {
                if(isNone(registers[inst.left]) || isNone(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare None with <=");
                }
                if(!isNumber(registers[inst.left]) || !isNumber(registers[inst.right])) {
                    throw std::runtime_error("Cannot compare non-number values with <=");
                }
                registers[inst.dst] = asNumber(registers[inst.left]) <= asNumber(registers[inst.right]) ? 1.0 : 0.0; break;
            }
            case OpCode::CMP_EQ:  registers[inst.dst] = registers[inst.left] == registers[inst.right] ? 1.0 : 0.0; break;
            case OpCode::CMP_NEQ: registers[inst.dst] = registers[inst.left] != registers[inst.right] ? 1.0 : 0.0; break;
            case OpCode::JZ: {
                    if(isFalsy(registers[inst.dst])) {
                        pc = getAddress(inst);
                        jumped = true;
                }
            }
            break;
            case OpCode::JMP: pc = getAddress(inst); jumped = true;
            break;
            case OpCode::PRINT: {
                const Value& val = registers[inst.dst];
                if (isString(val)) {
                    std::cout << asString(val);
                }
                else if (isNumber(val)) {
                    double num = asNumber(val);
                    if (num == (long long)num) {
                        std::cout << (long long)num;
                    } else {
                        std::cout << num;
                    }
                }
                else {
                    std::cout << valueToString(val);
                }
                break;
            }
            case OpCode::PRINT_STR: std::cout<<current_strings[inst.dst]; break;
            
            case OpCode::LOGICAL_AND: registers[inst.dst] = (isTruthy(registers[inst.left]) && isTruthy(registers[inst.right])) ? 1.0 : 0.0; break;
            case OpCode::LOGICAL_OR: registers[inst.dst] = (isTruthy(registers[inst.left]) || isTruthy(registers[inst.right])) ? 1.0 : 0.0; break;
            case OpCode::LOGICAL_NOT: registers[inst.dst] = isFalsy(registers[inst.left]) ? 1.0 : 0.0; break;
    
            case OpCode::CALL: {
                size_t retAddr = pc + 1;
                uint16_t funcAddr = getAddress(inst);
                callStack.push_back({retAddr, inst.dst, registers[2], registers[8], argBuffer, registers});
                argBuffer.clear();
    
                pc = funcAddr;
                jumped = true;
            }
            break;
    
            case OpCode::LENGTH: {
                const Value& val = registers[inst.left];
                if(isString(val)) {
                    registers[inst.dst] = static_cast<double>(asString(val).size());
                } else if(isArray(val)) {
                    registers[inst.dst] = static_cast<double>(asArray(val)->items.size());
                } else {
                    throw std::runtime_error("length() expects a string or array argument");
                }
            }
            break;

            // Generic subscript-read: dispatches on the runtime type of the
            // base value. Works for plain string/array indexing as well as
            // chained array indexing (matrix[i][j]).
            case OpCode::LOAD_STR_IDX: {
                const Value& baseVal = registers[inst.left];
                const Value& idxVal = registers[inst.right];
                if (isArray(baseVal)) {
                    auto& items = asArray(baseVal)->items;
                    int32_t idx = stringIndexFromValue(idxVal);
                    checkArrayBounds(items, idx);
                    registers[inst.dst] = items[static_cast<size_t>(idx)];
                } else if (isString(baseVal)) {
                    const std::string& str = asString(baseVal);
                    int32_t idx = stringIndexFromValue(idxVal);
                    checkStringBounds(str, idx);
                    registers[inst.dst] = std::string(1, str[static_cast<size_t>(idx)]);
                } else {
                    throw std::runtime_error("Subscript requires a string or array");
                }
            }
            break;

            // Generic subscript-write. Arrays are reference types, so the
            // mutation below is visible through every other reference to
            // the same array automatically. Strings are value types; the
            // compiler re-stores the mutated register back into its home
            // variable when the base was a plain variable (see compiler.cpp).
            case OpCode::STORE_STR_IDX: {
                Value& baseVal = registers[inst.left];
                const Value& idxVal = registers[inst.right];
                if (isArray(baseVal)) {
                    auto& items = asArray(baseVal)->items;
                    int32_t idx = stringIndexFromValue(idxVal);
                    checkArrayBounds(items, idx);
                    items[static_cast<size_t>(idx)] = registers[inst.dst];
                } else if (isString(baseVal)) {
                    char ch = singleCharFromValue(registers[inst.dst]);
                    int32_t idx = stringIndexFromValue(idxVal);
                    std::string& str = std::get<std::string>(baseVal);
                    checkStringBounds(str, idx);
                    str[static_cast<size_t>(idx)] = ch;
                } else {
                    throw std::runtime_error("Subscript assignment requires a string or array");
                }
            }
            break;

            case OpCode::ARRAY_NEW: {
                if (!isNumber(registers[inst.left])) {
                    throw std::runtime_error("array() expects a number argument");
                }
                double sizeD = asNumber(registers[inst.left]);
                if (sizeD < 0 || sizeD != std::floor(sizeD)) {
                    throw std::runtime_error("array() size must be a non-negative integer");
                }
                registers[inst.dst] = makeArray(static_cast<size_t>(sizeD));
            }
            break;

            case OpCode::ARRAY_LIT: {
                registers[inst.dst] = std::make_shared<ArrayObj>();
            }
            break;

            case OpCode::ARRAY_PUSH: {
                if (!isArray(registers[inst.left])) {
                    throw std::runtime_error("array_push() expects an array argument");
                }
                auto& items = asArray(registers[inst.left])->items;
                items.push_back(registers[inst.right]);
                registers[inst.dst] = static_cast<double>(items.size());
            }
            break;

            case OpCode::ARRAY_POP: {
                if (!isArray(registers[inst.left])) {
                    throw std::runtime_error("array_pop() expects an array argument");
                }
                auto& items = asArray(registers[inst.left])->items;
                if (items.empty()) {
                    throw std::runtime_error("array_pop() called on an empty array");
                }
                registers[inst.dst] = items.back();
                items.pop_back();
            }
            break;

            case OpCode::ARRAY_INSERT: {
                if (!isArray(registers[inst.left])) {
                    throw std::runtime_error("array_insert() expects an array argument");
                }
                auto& items = asArray(registers[inst.left])->items;
                int32_t idx = stringIndexFromValue(registers[inst.right]);
                if (idx < 0 || static_cast<size_t>(idx) > items.size()) {
                    throw std::runtime_error("array_insert() index out of bounds");
                }
                items.insert(items.begin() + idx, registers[inst.dst]);
            }
            break;

            case OpCode::ARRAY_REMOVE: {
                if (!isArray(registers[inst.left])) {
                    throw std::runtime_error("array_remove() expects an array argument");
                }
                auto& items = asArray(registers[inst.left])->items;
                int32_t idx = stringIndexFromValue(registers[inst.right]);
                checkArrayBounds(items, idx);
                registers[inst.dst] = items[static_cast<size_t>(idx)];
                items.erase(items.begin() + idx);
            }
            break;
    
            case OpCode::TYPE: {
                const Value& val = registers[inst.left];
                if(isString(val)) {
                    registers[inst.dst] = std::string("string");
                } else if(isNumber(val)) {
                    registers[inst.dst] = std::string("number");
                } else if(isNone(val)) {
                    registers[inst.dst] = std::string("none");
                } else if(isArray(val)) {
                    registers[inst.dst] = std::string("array");
                } else {
                    registers[inst.dst] = std::string("unknown");
                }
            }
            break;

            case OpCode::TO_NUMBER: {
                const Value& val = registers[inst.left];
                if(isNumber(val)) {
                    registers[inst.dst] = val;
                } else if(isString(val)) {
                    const std::string& str = asString(val);
                    try {
                        size_t pos = 0;
                        double result = std::stod(str, &pos);
                        while(pos < str.size() && std::isspace((unsigned char)str[pos])) pos++;
                        if(pos != str.size()) {
                            throw std::runtime_error("number(): cannot convert '" + str + "' to a number");
                        }
                        registers[inst.dst] = result;
                    } catch(const std::invalid_argument&) {
                        throw std::runtime_error("number(): cannot convert '" + str + "' to a number");
                    } catch(const std::out_of_range&) {
                        throw std::runtime_error("number(): '" + str + "' is out of range for a number");
                    }
                } else if(isNone(val)) {
                    throw std::runtime_error("number(): cannot convert none to a number");
                } else {
                    throw std::runtime_error("number(): cannot convert an array to a number");
                }
            }
            break;

            case OpCode::TO_STRING: {
                registers[inst.dst] = valueToString(registers[inst.left]);
            }
            break;
    
        case OpCode::ORD: {
            const Value& val = registers[inst.left];
            if (isString(val)) {
                const std::string& str = asString(val);
                if (str.empty()) {
                    registers[inst.dst] = 0.0;
                } else {
                    // UTF-8 decode first code point
                    unsigned char c = str[0];
                    int codePoint = c;

                    if ((c & 0xE0) == 0xC0 && str.size() >= 2) {
                        // 2-byte sequence
                        codePoint = ((c & 0x1F) << 6) | (str[1] & 0x3F);
                    } else if ((c & 0xF0) == 0xE0 && str.size() >= 3) {
                        // 3-byte sequence
                        codePoint = ((c & 0x0F) << 12) | ((str[1] & 0x3F) << 6) | (str[2] & 0x3F);
                    } else if ((c & 0xF8) == 0xF0 && str.size() >= 4) {
                        // 4-byte sequence
                        codePoint = ((c & 0x07) << 18) | ((str[1] & 0x3F) << 12) | ((str[2] & 0x3F) << 6) | (str[3] & 0x3F);
                    }

                    registers[inst.dst] = static_cast<double>(codePoint);
                }
            } else {
                throw std::runtime_error("ord() expects a string argument");
            }
        }
        break;
    
            case OpCode::CHR: {
                if(isNone(registers[inst.dst])) throw std::runtime_error("chr() expects a number argument, got none");
                int code = static_cast<int>(asNumber(registers[inst.left]));
    
                if(code < 0 || code > 255) {
                    throw std::runtime_error("chr() argument out of range (0-255)");
                }
                std::string result;
                if(code <= 0x7F) { // 1 byte
                    result = static_cast<char>(code);
                } else if(code <= 0x7FF) { // 2 bytes
                    result += static_cast<char>(0xC0 | (code >> 6));
                    result += static_cast<char>(0x80 | (code & 0x3F));
                } else if(code <= 0x7FFF) { // 3 bytes
                    result += static_cast<char>(0xE0 | (code >> 12));
                    result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (code & 0x3F));
                } else if(code <= 0x7FFFF) { // 4 bytes
                    result += static_cast<char>(0xF0 | (code >> 18));
                    result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
                    result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                    result += static_cast<char>(0x80 | (code & 0x3F));
                }
                registers[inst.dst] = result;
            }
            break;
    
            case OpCode::BIN: {
                if(isNone(registers[inst.left])) throw std::runtime_error("bin() expects a number");
                int value = toInt32(registers[inst.left]);
                std::string result;
                if(value == 0) result = "0";
                else {
                    unsigned int u = static_cast<unsigned int>(value);
                    while(u > 0) {
                        result = (char)('0' + (u % 2)) + result;
                        u /= 2;
                    }
                }
                registers[inst.dst] = "0b" + result;
            }
            break;
    
            case OpCode::HEX: {
                if(isNone(registers[inst.left])) throw std::runtime_error("hex() expects a number");
                int val = toInt32(registers[inst.left]);
                if(val == 0) {
                    registers[inst.dst] = "0x0";
                } else {
                    std::stringstream ss;
                    ss << "0x" << std::hex << val;
                    registers[inst.dst] = ss.str();
                }
            }
            break;
    
            case OpCode::OCT: {
                if(isNone(registers[inst.left])) throw std::runtime_error("oct() expects a number");
                int val = toInt32(registers[inst.left]);
                if(val == 0) {
                    registers[inst.dst] = "0o0";
                } else {
                    std::stringstream ss;
                    ss << "0o" << std::oct << val;
                    registers[inst.dst] = ss.str();
                }
            }
            break;
    
            case OpCode::DEC: {
                Value& val = registers[inst.left];
                if(isString(val)) {
                    const std::string& str = asString(val);
                    try {
                        long long result = 0;
                        bool negative = false;
                        std::string numStr = str;
    
                        if(numStr[0] == '-') {
                            negative = true;
                            numStr = numStr.substr(1);
                        }
                        // auto-detect base
                        if(numStr.size() > 2 && numStr[0] == '0' && (numStr[1] == 'b' || numStr[1] == 'B')) {
                            result = std::stoll(numStr.substr(2), nullptr, 2);
                        } else if(numStr.size() > 2 && numStr[0] == '0' && (numStr[1] == 'o' || numStr[1] == 'O')) {
                            result = std::stoll(numStr.substr(2), nullptr, 8);
                        } else if(numStr.size() > 2 && numStr[0] == '0' && (numStr[1] == 'x' || numStr[1] == 'X')) {
                            result = std::stoll(numStr.substr(2), nullptr, 16);
                        } else {
                            result = std::stoll(numStr);
                        }
    
                        if(negative) result = -result;
                        registers[inst.dst] = static_cast<double>(result);
                    } catch(...) {
                        registers[inst.dst] = 0.0;
                    }
                } else if(isNumber(val)) {
                    registers[inst.dst] = asNumber(val);
                } else {
                    registers[inst.dst] = 0.0;
                }
            }
            break;
            
            case OpCode::INPUT: {
                std::cout << std::flush;
                std::string inputStr;
 
                // EOF -> none
                if (!std::getline(std::cin, inputStr)) {
                    registers[inst.dst] = std::monostate{};
                    break;
                }
 
                // Strip Windows-style CR
                if (!inputStr.empty() && inputStr.back() == '\r') {
                    inputStr.pop_back();
                }
 
                // Empty line -> none
                if (inputStr.empty()) {
                    registers[inst.dst] = std::monostate{};
                    break;
                }
 
                // Trim leading/trailing whitespace
                size_t trimStart = inputStr.find_first_not_of(" \t");
                size_t trimEnd   = inputStr.find_last_not_of(" \t");
 
                // Step 3a: Only whitespace → none
                if (trimStart == std::string::npos) {
                    registers[inst.dst] = std::monostate{};
                    break;
                }
 
                std::string trimmed = inputStr.substr(trimStart, trimEnd - trimStart + 1);
 
                // Quoted string -> strip quotes, return as string (no number conversion)
                if (trimmed.size() >= 2) {
                    char first = trimmed.front(), last = trimmed.back();
                    if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                        registers[inst.dst] = trimmed.substr(1, trimmed.size() - 2);
                        break;
                    }
                }
 
                // No quotes -> try number conversion via strtod
                const char* str = trimmed.c_str();
                char* endPtr = nullptr;
                double numValue = std::strtod(str, &endPtr);
 
                // Consume any trailing whitespace after the number
                while (*endPtr == ' ' || *endPtr == '\t') {
                    ++endPtr;
                }
 
                // Full string consumed as number return double
                if (endPtr != str && *endPtr == '\0') {
                    registers[inst.dst] = numValue;
                } else {
                    // Not a valid number return as string (trimmed)
                    registers[inst.dst] = trimmed;
                }
            }
            break;
            case OpCode::SIN:   registers[inst.dst] = std::sin(asNumber(registers[inst.left])); break;
            case OpCode::COS:   registers[inst.dst] = std::cos(asNumber(registers[inst.left])); break;
            case OpCode::TAN:   registers[inst.dst] = std::tan(asNumber(registers[inst.left])); break;
            case OpCode::ASIN:  registers[inst.dst] = std::asin(asNumber(registers[inst.left])); break;
            case OpCode::ACOS:  registers[inst.dst] = std::acos(asNumber(registers[inst.left])); break;
            case OpCode::ATAN:  registers[inst.dst] = std::atan(asNumber(registers[inst.left])); break;
            case OpCode::ATAN2: registers[inst.dst] = std::atan2(asNumber(registers[inst.left]), asNumber(registers[inst.right])); break;
            case OpCode::SQRT:  registers[inst.dst] = std::sqrt(asNumber(registers[inst.left])); break;
            case OpCode::EXP:   registers[inst.dst] = std::exp(asNumber(registers[inst.left])); break;
            case OpCode::LOG:   registers[inst.dst] = std::log(asNumber(registers[inst.left])); break;
            case OpCode::LOG10: registers[inst.dst] = std::log10(asNumber(registers[inst.left])); break;
            case OpCode::CEIL:  registers[inst.dst] = std::ceil(asNumber(registers[inst.left])); break;
            case OpCode::FLOOR: registers[inst.dst] = std::floor(asNumber(registers[inst.left])); break;
            case OpCode::ABS:   registers[inst.dst] = std::fabs(asNumber(registers[inst.left])); break;
            case OpCode::ROUND: registers[inst.dst] = std::round(asNumber(registers[inst.left])); break;
            case OpCode::FMOD:  registers[inst.dst] = std::fmod(asNumber(registers[inst.left]), asNumber(registers[inst.right])); break;
            case OpCode::CBRT: registers[inst.dst] = std::cbrt(asNumber(registers[inst.left])); break;
            case OpCode::MATH_POW: registers[inst.dst] = std::pow(asNumber(registers[inst.left]), asNumber(registers[inst.right])); break;
            case OpCode::LOG2: registers[inst.dst] = std::log2(asNumber(registers[inst.left])); break;
            case OpCode::LOG_AB: registers[inst.dst] 
                = std::log(asNumber(registers[inst.right])) / std::log(asNumber(registers[inst.left])); break;
    
            case OpCode::RETURN: {
                    Value retVal = registers[inst.dst];
                    if(callStack.empty()) break;
                    CallFrame frame = callStack.back();
                    callStack.pop_back();
                    registers = frame.callerRegisters;
                    registers[2] = frame.callerSp;
                    registers[8] = frame.callerFp;
                    registers[frame.returnDest] = retVal;
                    pc = frame.returnAddress;
                    jumped = true;
            }
            break;
            
            case OpCode::PUSH_ARG:
                argBuffer.push_back(registers[inst.dst]);
            break;
            case OpCode::ARGC: {
                double n = callStack.empty() ? 0.0 : static_cast<double>(callStack.back().args.size());
                registers[inst.dst] = n;
            }
            break;
            case OpCode::COLLECT_VARARGS: {
                Value result;
                if (!callStack.empty()) {
                    const auto& args = callStack.back().args;
                    size_t startIdx = static_cast<size_t>(inst.left);
                    size_t variadicCount = (startIdx < args.size()) ? (args.size() - startIdx) : 0;
                    if (variadicCount == 1 && isArray(args[startIdx])) {
                        result = args[startIdx];
                    } else {
                        auto arr = std::make_shared<ArrayObj>();
                        if (variadicCount > 0) {
                            arr->items.assign(args.begin() + startIdx, args.end());
                        }
                        result = arr;
                    }
                } else {
                    result = makeArray(0);
                }
                registers[inst.dst] = result;
            }
            break;
            case OpCode::LOAD_PARAM: {
                if(!callStack.empty()) {
                    const auto& args = callStack.back().args;
                    registers[inst.dst] = inst.left < args.size() ? args[inst.left] : Value(0.0);
                }
            }
            break;
    
            case OpCode::LOAD: {
                int32_t base   = (int32_t)asNumber(registers[inst.left]); // FP
                int32_t offset = static_cast<int8_t>(inst.right);
                int32_t addr   = base + offset;
    
                if (addr < 0 || addr >= (int32_t)memory.size()) {
                    std::cerr << "LOAD out of range: addr=" << addr << std::endl;
                    throw std::runtime_error("LOAD out of range");
                }
                registers[inst.dst] = memory[addr];
                break;
            }
    
            case OpCode::STORE: {
                int32_t base   = (int32_t)asNumber(registers[inst.left]); // FP
                int32_t offset = static_cast<int8_t>(inst.right);
                int32_t addr   = base + offset;
    
                if (addr < 0 || addr >= (int32_t)memory.size()) {
                    std::cerr << "STORE out of range: addr=" << addr << std::endl;
                    throw std::runtime_error("STORE out of range");
                }
                memory[addr] = registers[inst.dst];
                break;
            }
    
            case OpCode::ADDI: {
                int8_t signedImm = static_cast<int8_t>(inst.right);
                int32_t imm = signedImm;
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) + imm);
                break;
            }
            case OpCode::ANDI: {
                int8_t signedImm = static_cast<int8_t>(inst.right);
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) & signedImm);
                break;
            }
            case OpCode::ORI: {
                int8_t signedImm = static_cast<int8_t>(inst.right);
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) | signedImm);
                break;
            }
            case OpCode::XORI: {
                int8_t signedImm = static_cast<int8_t>(inst.right);
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) ^ signedImm);
                break;
            }
            case OpCode::SLLI: {
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) << (inst.right & 0x1F));
                break;
            }
            case OpCode::SRLI: {
                registers[inst.dst] = fromInt32((uint32_t)toInt32(registers[inst.left]) >> (inst.right & 0x1F));
                break;
            }
            case OpCode::SRAI: {
                registers[inst.dst] = fromInt32(toInt32(registers[inst.left]) >> (inst.right & 0x1F));
                break;
            }
            case OpCode::LW:
                registers[inst.dst] = memory[inst.left];
                break;
            case OpCode::SW:
                memory[inst.left] = registers[inst.dst];
                break;
            case OpCode::RANDOM: {
                // inst.left == 0 && inst.right == 0  ->  range [0, 1)
                // otherwise  inst.left = minReg, inst.right = maxReg  ->  [min, max]
                if (inst.left == 0 && inst.right == 0) {
                    std::uniform_real_distribution<double> dist(0.0, 1.0);
                    registers[inst.dst] = dist(s_rng);
                } else {
                    double low = asNumber(registers[inst.left]);
                    double high = asNumber(registers[inst.right]);
                    if (low > high) std::swap(low, high);
                    std::uniform_real_distribution<double> dist(low, std::nextafter(high, std::numeric_limits<double>::infinity()));
                    registers[inst.dst] = dist(s_rng);
                }
            }
            break;
            default: break;
        }
    if (!jumped) pc++;
    return pc;
}

double VirtualMachine::run() {
    pc = 0;
    lastDestReg = 0;
    try {
        if (debug_mode && debugger) {
            debugger->run();
        } else {
            while (pc < current_program.size()) {
                executeSingleInstruction();
            }
        }
        if (isNumber(registers[lastDestReg])) {
            return asNumber(registers[lastDestReg]) == -0.0 ? 0.0 : asNumber(registers[lastDestReg]);
        }
        return 0.0;
    } catch (const std::exception& e) {
        int line = (pc < current_lineNumbers.size() ? current_lineNumbers[pc] : 0);
        throw std::runtime_error(
            "Line " + std::to_string(line) + ": " + e.what()
        );
    }
}