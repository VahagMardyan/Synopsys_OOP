#include "compiler.h"
#include <cmath>
#include <fstream>
#include <cstring>
#include <limits>

const int SP = 2;
const int FP = 8;

namespace {
    void emitStoreVariable(const std::shared_ptr<VariableNode>& var, int srcReg, std::vector<Instruction>& code);
}

static void rebaseJumpTargets(std::vector<Instruction>& instructions, uint16_t baseOffset) {
    for(auto& inst : instructions) {
        if(inst.op == (uint32_t)OpCode::JZ || inst.op == (uint32_t)OpCode::JMP) {
            setAddress(inst, static_cast<uint16_t>(getAddress(inst) + baseOffset));
        }
    }
}

// Validates a call's argument count against a function's declared shape:
// at least minParamCount, and (unless variadic) at most paramCount.
static void checkCallArgCount(const std::string& funcName, const FunctionInfo& info, size_t argCount) {
    if (argCount < static_cast<size_t>(info.minParamCount)) {
        throw std::runtime_error(
            "Function '" + funcName + "' requires at least " +
            std::to_string(info.minParamCount) + " argument(s), but " +
            std::to_string(argCount) + " provided"
        );
    }
    if (!info.hasVariadic && argCount > static_cast<size_t>(info.paramCount)) {
        throw std::runtime_error(
            "Function '" + funcName + "' accepts at most " +
            std::to_string(info.paramCount) + " argument(s), but " +
            std::to_string(argCount) + " provided"
        );
    }
}

void Compiler::emitMainPrologue(std::vector<Instruction>& code) {
    int slots = symTable.getProgramFrameSlotCount();
    if (slots < 1) slots = 1;
    int frameSize = (slots + 4) * 4;
    code.push_back({(uint32_t)OpCode::ADDI, SP, SP, (uint32_t)(int32_t)(-frameSize)});
    lineNumbers.push_back(0);
    code.push_back({(uint32_t)OpCode::ADDI, FP, SP, (uint32_t)frameSize});
    lineNumbers.push_back(0);
}

int Compiler::allocateTempRegister() {
    if(!freeRegisters.empty()) {
        int reg = freeRegisters.top();
        freeRegisters.pop();
        return reg;
    }
    while(nextTempIndex == SP || nextTempIndex == FP) {
        ++nextTempIndex;
    }
    return nextTempIndex++;
}

void Compiler::freeTempRegister(int reg) {
    if(reg != SP && reg != FP) {
        freeRegisters.push(reg);
    }
}

bool Compiler::tryEmitMathBuiltinCall(
    const std::string& name,
    const std::vector<std::shared_ptr<ASTNode>>& args,
    std::vector<Instruction>& code,
    int& resultReg,
    size_t pcBase
) {
    auto emitArg = [&](const std::shared_ptr<ASTNode>& arg) -> int {
        auto argCode = generateByteCode(postOrderTraverse(arg), pcBase + code.size());
        rebaseJumpTargets(argCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), argCode.begin(), argCode.end());
        return argCode.empty() ? 0 : static_cast<int>(argCode.back().dst);
    };

    auto emitUnary = [&](OpCode op) -> bool {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)op, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    };

    auto emitBinary = [&](OpCode op) -> bool {
        if(args.size() != 2) return false;
        int leftReg = emitArg(args[0]);
        int rightReg = emitArg(args[1]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)op, (uint32_t)resultReg, (uint32_t)leftReg, (uint32_t)rightReg});
        return true;
    };

    if(name == "sin") return emitUnary(OpCode::SIN);
    if(name == "cos") return emitUnary(OpCode::COS);
    if(name == "tan") return emitUnary(OpCode::TAN);
    if(name == "asin") return emitUnary(OpCode::ASIN);
    if(name == "acos") return emitUnary(OpCode::ACOS);
    if(name == "atan") return emitUnary(OpCode::ATAN);
    if(name == "atan2") return emitBinary(OpCode::ATAN2);
    if(name == "sqrt") return emitUnary(OpCode::SQRT);
    if(name == "cbrt") return emitUnary(OpCode::CBRT);
    if(name == "pow") return emitBinary(OpCode::MATH_POW);
    if(name == "exp") return emitUnary(OpCode::EXP);
    if(name == "log") return emitUnary(OpCode::LOG);
    if(name == "ln") return emitUnary(OpCode::LOG);
    if(name == "log10") return emitUnary(OpCode::LOG10);
    if(name == "log2") return emitUnary(OpCode::LOG2);
    if(name == "ceil") return emitUnary(OpCode::CEIL);
    if(name == "floor") return emitUnary(OpCode::FLOOR);
    if(name == "abs") return emitUnary(OpCode::ABS);
    if(name == "round") return emitUnary(OpCode::ROUND);
    if(name == "fmod") return emitBinary(OpCode::FMOD);
    if(name == "log_ab") return emitBinary(OpCode::LOG_AB);

    if(name == "length") return emitUnary(OpCode::LENGTH);

    if(name == "array") {
        if(args.size() == 1) {
            // array(n) -> new array of size n, filled with none (unchanged)
            int sizeReg = emitArg(args[0]);
            resultReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::ARRAY_NEW, (uint32_t)resultReg, (uint32_t)sizeReg, 0});
            freeTempRegister(sizeReg);
            return true;
        }
        // array() -> [] , and array(a, b, c, ...) -> [a, b, c, ...]
        // Same ARRAY_LIT + ARRAY_PUSH-per-element shape the [...] literal
        // compiles to (see the ArrayLiteralNode case in generateByteCode).
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::ARRAY_LIT, (uint32_t)resultReg, 0, 0});
        for(const auto& elem : args) {
            int elemReg = emitArg(elem);
            code.push_back({(uint32_t)OpCode::ARRAY_PUSH, (uint32_t)elemReg, (uint32_t)resultReg, (uint32_t)elemReg});
            freeTempRegister(elemReg);
        }
        // Some callers (e.g. print()'s statement-level codegen) take a
        // shortcut and read the LAST emitted instruction's dst as "the"
        // result register, instead of going through tryEmitMathBuiltinCall's
        // resultReg out-param. ARRAY_PUSH's dst is a scratch/discarded slot,
        // so restore that invariant with a harmless self-MOV - the same fix
        // the [...] array-literal codegen already applies for this reason.
        code.push_back({(uint32_t)OpCode::MOV, (uint32_t)resultReg, (uint32_t)resultReg, 0});
        return true;
    }

    if(name == "number") return emitUnary(OpCode::TO_NUMBER);
    if(name == "string") return emitUnary(OpCode::TO_STRING);

    if(name == "array_push") {
        // array_push(arr, value) -> mutates arr in place, returns new length
        if(args.size() != 2) return false;
        int arrReg = emitArg(args[0]);
        int valReg = emitArg(args[1]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::ARRAY_PUSH, (uint32_t)resultReg, (uint32_t)arrReg, (uint32_t)valReg});
        freeTempRegister(arrReg);
        freeTempRegister(valReg);
        return true;
    }

    if(name == "array_pop") {
        // array_pop(arr) -> mutates arr in place, returns removed last element
        if(args.size() != 1) return false;
        int arrReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::ARRAY_POP, (uint32_t)resultReg, (uint32_t)arrReg, 0});
        freeTempRegister(arrReg);
        return true;
    }

    if(name == "array_insert") {
        // array_insert(arr, idx, value) -> mutates arr in place
        if(args.size() != 3) return false;
        int arrReg = emitArg(args[0]);
        int idxReg = emitArg(args[1]);
        int valReg = emitArg(args[2]);
        // Mirrors STORE_STR_IDX's operand layout: dst=value, left=array, right=index.
        code.push_back({(uint32_t)OpCode::ARRAY_INSERT, (uint32_t)valReg, (uint32_t)arrReg, (uint32_t)idxReg});
        freeTempRegister(arrReg);
        freeTempRegister(idxReg);
        resultReg = valReg; // expression value = the inserted value (already in valReg)
        return true;
    }

    if(name == "array_remove") {
        // array_remove(arr, idx) -> mutates arr in place, returns removed element
        if(args.size() != 2) return false;
        int arrReg = emitArg(args[0]);
        int idxReg = emitArg(args[1]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::ARRAY_REMOVE, (uint32_t)resultReg, (uint32_t)arrReg, (uint32_t)idxReg});
        freeTempRegister(arrReg);
        freeTempRegister(idxReg);
        return true;
    }

    if(name == "input") {
        resultReg = allocateTempRegister();

        // Print each prompt argument in order (same treatment as print():
        // string literals go straight to the string pool via PRINT_STR,
        // everything else is evaluated and printed with PRINT).
        for(const auto& promptArg : args) {
            if(auto strNode = std::dynamic_pointer_cast<StringNode>(promptArg)) {
                const std::string& val = strNode->getValue();
                int strIdx;
                auto it = stringMap.find(val);
                if(it != stringMap.end()) {
                    strIdx = it->second;
                } else {
                    strIdx = (int)stringPool.size();
                    stringPool.push_back(val);
                    stringMap[val] = strIdx;
                }
                code.push_back({(uint32_t)OpCode::PRINT_STR, (uint32_t)strIdx, 0, 0});
            } else {
                int promptReg = emitArg(promptArg);
                code.push_back({(uint32_t)OpCode::PRINT, (uint32_t)promptReg, 0, 0});
                freeTempRegister(promptReg);
            }
        }
        code.push_back({(uint32_t)OpCode::INPUT, (uint32_t)resultReg, 0, 0});
        return true;
    }

    if(name == "type") {
        if(args.size() != 1) return false;

        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::TYPE, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }

    if(name == "ord") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::ORD, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }

    if(name == "chr") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::CHR, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }

    if(name == "bin") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::BIN, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }

    if(name == "hex") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::HEX, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }
    if(name == "oct") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::OCT, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }
    if(name == "dec") {
        if(args.size() != 1) return false;
        int argReg = emitArg(args[0]);
        resultReg = allocateTempRegister();
        code.push_back({(uint32_t)OpCode::DEC, (uint32_t)resultReg, (uint32_t)argReg, 0});
        return true;
    }
    if(name == "random") {
        if(args.size() == 0) {
            // random() -> [0, 1)
            resultReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::RANDOM, (uint32_t)resultReg, 0, 0});
            return true;
        } else if(args.size() == 2) {
            // random(min, max) -> [min, max]
            int leftReg  = emitArg(args[0]);
            int rightReg = emitArg(args[1]);
            resultReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::RANDOM, (uint32_t)resultReg,
                            (uint32_t)leftReg, (uint32_t)rightReg});
            freeTempRegister(leftReg);
            freeTempRegister(rightReg);
            return true;
        }
        return false; // wrong arg count
    }

    return false;
}

std::vector<std::shared_ptr<ASTNode>> Compiler::postOrderTraverse(std::shared_ptr<ASTNode> root) {
    if(!root) return {};
    std::vector<std::shared_ptr<ASTNode>> postOrder;
    std::stack<std::shared_ptr<ASTNode>> s1, s2;
    s1.push(root);
    while(!s1.empty()) {
        auto node = s1.top(); s1.pop(); s2.push(node);
        if(std::dynamic_pointer_cast<TernaryOpNode>(node)) {
            continue;
        }
        if(auto binShortCircuit = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
            const std::string& opStr = binShortCircuit->getOp();
            if(opStr == "and" || opStr == "or") {
                continue; // compiled specially in generateByteCode with jumps
            }
        }
        for(auto& child : node->getChildren()) s1.push(child);
    }
    while(!s2.empty()) { postOrder.push_back(s2.top()); s2.pop(); }
    return postOrder;
}

std::shared_ptr<ASTNode> Compiler::optimize(std::shared_ptr<ASTNode> node) {
    if(!node) return nullptr;
    if(auto bin = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
        auto left  = optimize(bin->getLeft());
        auto right = optimize(bin->getRight());
        auto lNum  = std::dynamic_pointer_cast<NumberNode>(left);
        auto rNum  = std::dynamic_pointer_cast<NumberNode>(right);
        if(lNum && rNum) {
            double v1 = lNum->getValue(), v2 = rNum->getValue(), result = 0;
            switch(bin->getOpCode()) {
                case OpCode::ADD: result = v1+v2; break;
                case OpCode::SUB: result = v1-v2; break;
                case OpCode::MUL: result = v1*v2; break;
                case OpCode::POW: result = std::pow(v1, v2); break;
                case OpCode::DIV:
                    if (v2 == 0) return std::make_shared<BinaryOpNode>(bin->getOp(), left, right);
                    result = v1/v2;
                    break;
                case OpCode::FLOOR_DIV:
                    if (v2 == 0) return std::make_shared<BinaryOpNode>(bin->getOp(), left, right);
                    result = std::floor(v1/v2);
                    break;
                case OpCode::FRAC_DIV:
                    if (v2 == 0) return std::make_shared<BinaryOpNode>(bin->getOp(), left, right);
                    result = (v1 / v2 - std::floor(v1/v2));
                    break;
                case OpCode::AND: result = (double)((long long)v1 & (long long)v2); break;
                case OpCode::OR:  result = (double)((long long)v1 | (long long)v2); break;
                case OpCode::XOR: result = (double)((long long)v1 ^ (long long)v2); break;
                case OpCode::MODULO:
                    if ((long long)v2 == 0) return std::make_shared<BinaryOpNode>(bin->getOp(), left, right);
                    result = (double)((long long)v1 % (long long)v2);
                    break;
                case OpCode::SLL: result = (double)((long long)v1 << (long long)v2); break;
                case OpCode::SRL: result = (double)((uint32_t)((long long)v1) >> (((long long)v2) & 0x1F)); break;
                case OpCode::LOGICAL_AND: result = (v1 != 0 && v2 != 0) ? 1.0 : 0.0; break;
                case OpCode::LOGICAL_OR: result = (v1 != 0 || v2 != 0) ? 1.0 : 0.0; break;
                case OpCode::SLT:
                case OpCode::CMP_LT:
                    result = (v1 < v2) ? 1.0 : 0.0;
                    break;
                case OpCode::CMP_LET: result = (v1 <= v2) ? 1.0 : 0.0; break;
                case OpCode::CMP_GT: result = (v1 > v2) ? 1.0 : 0.0; break;
                case OpCode::CMP_GET: result = (v1 >= v2) ? 1.0 : 0.0; break;
                case OpCode::CMP_EQ: result = (v1 == v2) ? 1.0 : 0.0; break;
                case OpCode::CMP_NEQ: result = (v1 != v2) ? 1.0 : 0.0; break;
                case OpCode::CONST_E: result = 2.718281828459045; break;
                case OpCode::CONST_PI: result = 3.14159265358979323846; break;
                case OpCode::CONST_INF: result = std::numeric_limits<double>::infinity(); break;
                case OpCode::CONST_MAX: result = std::numeric_limits<double>::max(); break;
                default: break;
            }
            return std::make_shared<NumberNode>(result);
        }
        return std::make_shared<BinaryOpNode>(bin->getOp(), left, right);
    }
    if(auto un = std::dynamic_pointer_cast<UnaryOpNode>(node)) {
        auto child = optimize(un -> getChild());
        auto num = std::dynamic_pointer_cast<NumberNode>(child);
        if(num) {
            if(un -> getOp() == "-" || un -> getOp() == "_") {
                return std::make_shared<NumberNode>(-num -> getValue());
            }
            if(un -> getOp() == "+" || un -> getOp() == "#") {
                return num;
            }
            if(un -> getOp() == "not") {
                return std::make_shared<NumberNode>(num -> getValue() == 0 ? 1.0 : 0.0);
            }
            if(un -> getOp() == "~") {
                long long val = static_cast<long long>(num -> getValue());
                return std::make_shared<NumberNode>(static_cast<double>(~val));
            }
        }
        return std::make_shared<UnaryOpNode>(un -> getOp(), child);
    }
    if(auto block = std::dynamic_pointer_cast<BlockCode>(node)) {
        auto optimizedBlock = std::make_shared<BlockCode>();
        optimizedBlock->lineNumber = block->lineNumber;
        for(auto& s : block->getStatements()) {
            auto optStmt = std::dynamic_pointer_cast<StatementNode>(optimize(s));
            if(optStmt) optimizedBlock->addStatement(optStmt);
        }
        return optimizedBlock;
    }
    if(auto assign = std::dynamic_pointer_cast<AssignmentNode>(node)) {
        std::shared_ptr<AssignmentNode> n;
        if (assign->isLocal()) {
            n = std::make_shared<AssignmentNode>(assign->getOffset(), optimize(assign->getValue()), assign->getLocalOuterHops());
        } else {
            n = std::make_shared<AssignmentNode>(assign->getAddress(), optimize(assign->getValue()));
        }
        n->lineNumber = assign->lineNumber;
        return n;
    }
    if(auto ifStmt = std::dynamic_pointer_cast<IfStatementNode>(node)) {
        auto cond   = optimize(ifStmt->getCondition());
        auto thenBr = std::dynamic_pointer_cast<StatementNode>(optimize(ifStmt->getThenBr()));
        auto elseBr = std::dynamic_pointer_cast<StatementNode>(optimize(ifStmt->getElseBr()));
        if(auto condNum = std::dynamic_pointer_cast<NumberNode>(cond))
            return condNum->getValue() != 0 ? thenBr : (elseBr ? elseBr : nullptr);
        auto n = std::make_shared<IfStatementNode>(cond, thenBr, elseBr);
        n->lineNumber = ifStmt->lineNumber;
        return n;
    }
    if(auto whileStmt = std::dynamic_pointer_cast<WhileStatementNode>(node)) {
        auto cond = optimize(whileStmt->getCondition());
        auto body = std::dynamic_pointer_cast<StatementNode>(optimize(whileStmt->getBody()));
        if(auto condNum = std::dynamic_pointer_cast<NumberNode>(cond))
            if(condNum->getValue() == 0) return nullptr;
        auto n = std::make_shared<WhileStatementNode>(cond, body);
        n->lineNumber = whileStmt->lineNumber;
        return n;
    }
    if(auto printStmt = std::dynamic_pointer_cast<PrintNode>(node)) {
        std::vector<std::shared_ptr<ASTNode>> exprs;
        for(const auto& e : printStmt->getExpressions()) exprs.push_back(optimize(e));
        auto n = std::make_shared<PrintNode>(std::move(exprs));
        n->lineNumber = printStmt->lineNumber;
        return n;
    }
    if(auto forStmt = std::dynamic_pointer_cast<ForStatementNode>(node)) {
        auto init = std::dynamic_pointer_cast<StatementNode>(optimize(forStmt -> getInit()));
        auto cond = optimize(forStmt -> getCondition());
        auto update = std::dynamic_pointer_cast<StatementNode>(optimize(forStmt -> getUpdate()));
        auto body = std::dynamic_pointer_cast<StatementNode>(optimize(forStmt -> getBody()));
        auto n = std::make_shared<ForStatementNode>(init, cond, update, body);
        n->lineNumber = forStmt->lineNumber;
        return n;
    }
    if(auto strNode = std::dynamic_pointer_cast<StringNode>(node)) {
        return strNode;
    }
    if(auto sub = std::dynamic_pointer_cast<SubscriptReadNode>(node)) {
        return std::make_shared<SubscriptReadNode>(
            optimize(sub->getObject()), optimize(sub->getIndex()));
    }
    if(auto subWrite = std::dynamic_pointer_cast<SubscriptWriteNode>(node)) {
        auto n = std::make_shared<SubscriptWriteNode>(
            optimize(subWrite->getObject()),
            optimize(subWrite->getIndex()),
            optimize(subWrite->getValue()));
        n->lineNumber = subWrite->lineNumber;
        return n;
    }
    if(auto arrLit = std::dynamic_pointer_cast<ArrayLiteralNode>(node)) {
        std::vector<std::shared_ptr<ASTNode>> elems;
        for(const auto& e : arrLit->getElements()) elems.push_back(optimize(e));
        return std::make_shared<ArrayLiteralNode>(std::move(elems));
    }
    return node;
}

ByteCode Compiler::compile(
    std::shared_ptr<ASTNode> root,
    bool allowUnresolvedCalls,
    bool emitMainFramePrologue
) {
    constantPool.clear();
    lineNumbers.clear();
    stringPool.clear();
    stringMap.clear();
    constMap.clear();
    nextTempIndex = 0;
    functionTable.clear();
    forwardCalls.clear();
    
    std::vector<Instruction> insts;
    if(!root) {
        ByteCode empty;
        empty.instructions = insts;
        empty.constants = constantPool;
        empty.strings = stringPool;
        empty.globalSlotCount = symTable.getGlobalSlotCount();
        empty.globalNamesBySlot.assign(empty.globalSlotCount, std::string{});
        for (const auto& [name, addr] : symTable.getGlobalAddressMap()) {
            if (addr < empty.globalNamesBySlot.size()) {
                empty.globalNamesBySlot[addr] = name;
            }
        }
        return empty;
    }
    
    auto optimizedRoot = optimize(root);
    if(emitMainFramePrologue) {
        emitMainPrologue(insts);
    }

    if (auto block = std::dynamic_pointer_cast<BlockCode>(optimizedRoot)) {
        for (auto& s : block->getStatements()) {
            compileStatement(s, insts);
        }
    } else if (auto stmt = std::dynamic_pointer_cast<StatementNode>(optimizedRoot)) {
        compileStatement(stmt, insts);
    }

    if(!allowUnresolvedCalls) {
        auto mainIt = functionTable.find("main");
        if(mainIt == functionTable.end()) {
            throw std::runtime_error("Program must define function main()");
        }
        int resultReg = allocateTempRegister();
        Instruction callMain;
        callMain.op = (uint32_t)OpCode::CALL;
        callMain.dst = (uint32_t)resultReg;
        setAddress(callMain, (uint16_t)mainIt->second.address);
        insts.push_back(callMain);
        lineNumbers.push_back(0);
        freeTempRegister(resultReg);
    }

    std::vector<std::pair<size_t, std::string>> unresolvedCalls;
    for(auto& [idx, name] : forwardCalls) {
        if(functionTable.count(name)) {
            setAddress(insts[idx], (uint16_t)functionTable[name].address);
        } else if(!allowUnresolvedCalls) {
            throw std::runtime_error("Undefined function: " + name);
        } else {
            unresolvedCalls.push_back({idx, name});
        }
    }

    ByteCode bc;
    while(lineNumbers.size() < insts.size()) {
        int fallBack = lineNumbers.empty() ? 0 : lineNumbers.back();
        lineNumbers.push_back(fallBack);
    }

    if(lineNumbers.size() > insts.size()) {
        lineNumbers.reserve(insts.size());
    }

    bc.instructions = std::move(insts);
    bc.constants = constantPool;
    bc.strings = stringPool;
    bc.lineNumbers = std::move(lineNumbers);
    for(const auto& [name, info] : functionTable) {
        bc.functionSymbols[name] = info.address;
    }
    bc.unresolvedCalls = std::move(unresolvedCalls);
    bc.globalSlotCount = symTable.getGlobalSlotCount();
    bc.globalNamesBySlot.assign(bc.globalSlotCount, std::string{});
    for (const auto& [name, addr] : symTable.getGlobalAddressMap()) {
        if (addr < bc.globalNamesBySlot.size()) {
            bc.globalNamesBySlot[addr] = name;
        }
    }
    return bc;
}

std::vector<Instruction> Compiler::generateByteCode(
    const std::vector<std::shared_ptr<ASTNode>>& nodes,
    size_t pcBase) {
    std::vector<Instruction> code;
    std::stack<int> storage;
    
    for(const auto& node : nodes) {
        if(auto num = std::dynamic_pointer_cast<NumberNode>(node)) {
            double val = num -> getValue();
            int reg = allocateTempRegister();
            int idx;
            auto it = constMap.find(val);
            if(it != constMap.end()) {
                idx = it->second;
            } else {
                idx = (int)constantPool.size();
                constantPool.push_back(val);
                constMap[val] = idx;
            }
            code.push_back({(uint32_t)OpCode::LOAD_CONST, (uint32_t)reg, (uint32_t)idx, 0});
            storage.push(reg);
        }
        else if(auto var = std::dynamic_pointer_cast<VariableNode>(node)) {
            int rd = allocateTempRegister();
            if (var->getIsLocal()) {
                int32_t off = var->getLocalOffset();
                int oh = var->getOuterHops();
                if (oh > 0) {
                    code.push_back({(uint32_t)OpCode::LOAD_OUTER, (uint32_t)rd, (uint32_t)oh,
                        (uint32_t)(uint8_t)(int8_t)off});
                } else {
                    code.push_back({(uint32_t)OpCode::LOAD, (uint32_t)rd, (uint32_t)FP, (uint32_t)off});
                }
            } else {
                size_t addr = var->getGlobalAddr();
                code.push_back({(uint32_t)OpCode::LOAD_VAR, (uint32_t)rd, (uint32_t)addr, 0});
            }
            storage.push(rd);
        }
        else if(auto bin = std::dynamic_pointer_cast<BinaryOpNode>(node);
                bin && (bin->getOp() == "and" || bin->getOp() == "or")) {
            // Short-circuit and/or: the right operand's bytecode must only
            // run when the left side didn't already decide the result.
            // Its children were skipped by postOrderTraverse (see above),
            // so we generate+splice each side's code ourselves, the same
            // way the ternary operator does, using JZ + JMP for branching.
            bool isAnd = (bin->getOp() == "and");
            int resultReg = allocateTempRegister();

            auto leftCode = generateByteCode(postOrderTraverse(bin->getLeft()), pcBase + code.size());
            rebaseJumpTargets(leftCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), leftCode.begin(), leftCode.end());
            int leftReg = leftCode.empty() ? 0 : leftCode.back().dst;

            size_t jzIdx = code.size();
            code.push_back({(uint32_t)OpCode::JZ, (uint32_t)leftReg, 0, 0});

            if(isAnd) {
                // left truthy -> fall through and evaluate right, coercing
                // it to a canonical 0.0/1.0 via a double LOGICAL_NOT.
                auto rightCode = generateByteCode(postOrderTraverse(bin->getRight()), pcBase + code.size());
                rebaseJumpTargets(rightCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), rightCode.begin(), rightCode.end());
                int rightReg = rightCode.empty() ? 0 : rightCode.back().dst;

                int tmp = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::LOGICAL_NOT, (uint32_t)tmp, (uint32_t)rightReg, 0});
                code.push_back({(uint32_t)OpCode::LOGICAL_NOT, (uint32_t)resultReg, (uint32_t)tmp, 0});
                freeTempRegister(tmp);
                freeTempRegister(rightReg);

                size_t jmpIdx = code.size();
                code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
                setAddress(code[jzIdx], (uint16_t)code.size());

                // left falsy -> result is false, right never evaluated
                int zeroIdx;
                auto zit = constMap.find(0.0);
                if(zit != constMap.end()) zeroIdx = zit->second;
                else { zeroIdx = (int)constantPool.size(); constantPool.push_back(0.0); constMap[0.0] = zeroIdx; }
                code.push_back({(uint32_t)OpCode::LOAD_CONST, (uint32_t)resultReg, (uint32_t)zeroIdx, 0});

                setAddress(code[jmpIdx], (uint16_t)code.size());
            } else {
                // left truthy -> result is true, right never evaluated
                int oneIdx;
                auto oit = constMap.find(1.0);
                if(oit != constMap.end()) oneIdx = oit->second;
                else { oneIdx = (int)constantPool.size(); constantPool.push_back(1.0); constMap[1.0] = oneIdx; }
                code.push_back({(uint32_t)OpCode::LOAD_CONST, (uint32_t)resultReg, (uint32_t)oneIdx, 0});

                size_t jmpIdx = code.size();
                code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
                setAddress(code[jzIdx], (uint16_t)code.size());

                // left falsy -> evaluate right, coercing to canonical 0.0/1.0
                auto rightCode = generateByteCode(postOrderTraverse(bin->getRight()), pcBase + code.size());
                rebaseJumpTargets(rightCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), rightCode.begin(), rightCode.end());
                int rightReg = rightCode.empty() ? 0 : rightCode.back().dst;

                int tmp = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::LOGICAL_NOT, (uint32_t)tmp, (uint32_t)rightReg, 0});
                code.push_back({(uint32_t)OpCode::LOGICAL_NOT, (uint32_t)resultReg, (uint32_t)tmp, 0});
                freeTempRegister(tmp);
                freeTempRegister(rightReg);

                setAddress(code[jmpIdx], (uint16_t)code.size());
            }

            freeTempRegister(leftReg);
            storage.push(resultReg);
        }
        else if(auto bin = std::dynamic_pointer_cast<BinaryOpNode>(node)) {
            int r = storage.top(); storage.pop();
            int l = storage.top(); storage.pop();
            int target = allocateTempRegister();
            code.push_back({(uint32_t)bin->getOpCode(), (uint32_t)target, (uint32_t)l, (uint32_t)r});
            freeTempRegister(l);
            freeTempRegister(r);
            storage.push(target);
        }
        else if(auto un = std::dynamic_pointer_cast<UnaryOpNode>(node)) {
            int childIdx = storage.top(); storage.pop();
            int target = allocateTempRegister();
            OpCode opcode;
            if(un -> getOp() == "not") {
                opcode = OpCode::LOGICAL_NOT;
            } else if(un -> getOp() == "~") {
                opcode = OpCode::NOT;
            } else {
                opcode = OpCode::UNARY;
            }
            code.push_back({(uint32_t)opcode, (uint32_t)target, (uint32_t)childIdx, 0});
            freeTempRegister(childIdx);
            storage.push(target);
        }
        else if(auto strNode = std::dynamic_pointer_cast<StringNode>(node)) {
            const std::string& val = strNode->getValue();
            int strIdx;
            auto it = stringMap.find(val);
            if(it != stringMap.end()) {
                strIdx = it->second;
            } else {
                strIdx = (int)stringPool.size();
                stringPool.push_back(val);
                stringMap[val] = strIdx;
            }
            int reg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::LOAD_STR, (uint32_t)reg, (uint32_t)strIdx, 0});
            storage.push(reg);
        }
        else if(auto callExpr = std::dynamic_pointer_cast<FunctionCallNode>(node)) {
            int builtinResultReg = 0;
            if(tryEmitMathBuiltinCall(callExpr->getName(), callExpr->getArgs(), code, builtinResultReg, pcBase)) {
                storage.push(builtinResultReg);
                continue;
            }

            // Argument count check
            auto it = functionTable.find(callExpr->getName());
            if (it != functionTable.end()) {
                checkCallArgCount(callExpr->getName(), it->second, callExpr->getArgs().size());
            }

            for(const auto& arg : callExpr->getArgs()) {
                auto argCode = generateByteCode(postOrderTraverse(arg), pcBase + code.size());
                rebaseJumpTargets(argCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), argCode.begin(), argCode.end());
                int argReg = argCode.empty() ? 0 : argCode.back().dst;
                code.push_back({(uint32_t)OpCode::PUSH_ARG, (uint32_t)argReg, 0, 0});
                freeTempRegister(argReg);
            }
        
            int resultReg = allocateTempRegister();
            Instruction callInst;
            callInst.op  = (uint32_t)OpCode::CALL;
            callInst.dst = (uint32_t)resultReg;
            if(functionTable.count(callExpr->getName())) {
                setAddress(callInst, (uint16_t)functionTable[callExpr->getName()].address);
            } else {
                setAddress(callInst, 0);
                forwardCalls.push_back({pcBase + code.size(), callExpr->getName()});
            }
            code.push_back(callInst);
            storage.push(resultReg);
        }
        else if(auto mathConst = std::dynamic_pointer_cast<MathConstantNode>(node)) {
            int reg = allocateTempRegister();
            code.push_back({(uint32_t)mathConst->getConstant(), (uint32_t)reg, 0, 0});
            storage.push(reg);
        } else if(auto ternary = std::dynamic_pointer_cast<TernaryOpNode>(node)) {
            int resultReg = allocateTempRegister();
                
            // Generate condition
            auto condCode = generateByteCode(postOrderTraverse(ternary->getCondition()), pcBase + code.size());
            rebaseJumpTargets(condCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), condCode.begin(), condCode.end());
            int condReg = condCode.empty() ? 0 : condCode.back().dst;
                
            // Generate true branch and store to resultReg
            size_t jzIdx = code.size();
            code.push_back({(uint32_t)OpCode::JZ, (uint32_t)condReg, 0, 0});
                
            auto trueCode = generateByteCode(postOrderTraverse(ternary->getTrueExpr()), pcBase + code.size());
            rebaseJumpTargets(trueCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), trueCode.begin(), trueCode.end());
            int trueReg = trueCode.empty() ? 0 : trueCode.back().dst;
            code.push_back({(uint32_t)OpCode::MOV, (uint32_t)resultReg, (uint32_t)trueReg, 0});
            freeTempRegister(trueReg);
                
            size_t jmpIdx = code.size();
            code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
            setAddress(code[jzIdx], (uint16_t)code.size());
                
            // Generate false branch and store to resultReg
            auto falseCode = generateByteCode(postOrderTraverse(ternary->getFalseExpr()), pcBase + code.size());
            rebaseJumpTargets(falseCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), falseCode.begin(), falseCode.end());
            int falseReg = falseCode.empty() ? 0 : falseCode.back().dst;
            code.push_back({(uint32_t)OpCode::MOV, (uint32_t)resultReg, (uint32_t)falseReg, 0});
            freeTempRegister(falseReg);
                
            setAddress(code[jmpIdx], (uint16_t)code.size());
            freeTempRegister(condReg);
                
            storage.push(resultReg);
        } else if(auto noneNode = std::dynamic_pointer_cast<NoneNode>(node)) {
            int reg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::LOAD_NONE, (uint32_t)reg, 0, 0});
            storage.push(reg);
        }
        else if(auto sub = std::dynamic_pointer_cast<SubscriptReadNode>(node)) {
            // Generic subscript-read: LOAD_STR_IDX dispatches on the runtime
            // type of the base value (string -> char, array -> element), so
            // this same path handles both string indexing and array
            // indexing/chained indexing (e.g. matrix[i][j]).
            int idxReg = storage.top(); storage.pop();
            int strReg = storage.top(); storage.pop();
            int dst = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::LOAD_STR_IDX, (uint32_t)dst, (uint32_t)strReg, (uint32_t)idxReg});
            freeTempRegister(strReg);
            freeTempRegister(idxReg);
            storage.push(dst);
        }
        else if(auto arrLit = std::dynamic_pointer_cast<ArrayLiteralNode>(node)) {
            // Create the (empty) array first, then append each element
            // immediately after it is computed. Deliberately NOT routed
            // through the shared PUSH_ARG/argBuffer mechanism used for call
            // arguments: a nested array literal element (matrix rows) would
            // itself need that same buffer while it's still "in flight" for
            // the outer literal, corrupting it. Pushing straight into this
            // array's own storage as we go avoids any shared, global state.
            int arrReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::ARRAY_LIT, (uint32_t)arrReg, 0, 0});
            for(const auto& elem : arrLit->getElements()) {
                auto elemCode = generateByteCode(postOrderTraverse(elem), pcBase + code.size());
                rebaseJumpTargets(elemCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), elemCode.begin(), elemCode.end());
                int elemReg = elemCode.empty() ? 0 : elemCode.back().dst;
                // dst==right is safe: the pushed value is read before the
                // (discarded) new-length result overwrites the same slot.
                code.push_back({(uint32_t)OpCode::ARRAY_PUSH, (uint32_t)elemReg, (uint32_t)arrReg, (uint32_t)elemReg});
                freeTempRegister(elemReg);
            }
            // Callers rely on the convention that the LAST emitted
            // instruction's `dst` field names the register holding this
            // expression's result; ARRAY_PUSH's dst is a scratch/discarded
            // slot, so restore that invariant with a harmless self-MOV.
            code.push_back({(uint32_t)OpCode::MOV, (uint32_t)arrReg, (uint32_t)arrReg, 0});
            storage.push(arrReg);
        }
        else if(auto walrus = std::dynamic_pointer_cast<WalrusNode>(node)) {
            auto valueCode = generateByteCode(postOrderTraverse(walrus->getValue()), pcBase + code.size());
            rebaseJumpTargets(valueCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), valueCode.begin(), valueCode.end());
            int valReg = valueCode.empty() ? 0 : valueCode.back().dst;

            std::shared_ptr<VariableNode> targetVar = walrus->isLocal()
                ? std::make_shared<VariableNode>(walrus->getOffset(), walrus->getLocalOuterHops())
                : std::make_shared<VariableNode>(walrus->getAddress());
            emitStoreVariable(targetVar, valReg, code);

            storage.push(valReg);
        }
        else if(auto fstr = std::dynamic_pointer_cast<FStringNode>(node)) {
            const auto& literals = fstr->getLiterals();
            const auto& exprs = fstr->getExpressions();

            auto internString = [&](const std::string& val) -> int {
                auto it = stringMap.find(val);
                if(it != stringMap.end()) return it->second;
                int idx = (int)stringPool.size();
                stringPool.push_back(val);
                stringMap[val] = idx;
                return idx;
            };

            int resultReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::LOAD_STR, (uint32_t)resultReg, (uint32_t)internString(literals[0]), 0});

            for(size_t i = 0; i < exprs.size(); ++i) {
                auto exprCode = generateByteCode(postOrderTraverse(exprs[i]), pcBase + code.size());
                rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), exprCode.begin(), exprCode.end());
                int exprReg = exprCode.empty() ? 0 : exprCode.back().dst;

                int strReg = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::TO_STRING, (uint32_t)strReg, (uint32_t)exprReg, 0});
                freeTempRegister(exprReg);

                int nextResultReg = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::ADD, (uint32_t)nextResultReg, (uint32_t)resultReg, (uint32_t)strReg});
                freeTempRegister(resultReg);
                freeTempRegister(strReg);
                resultReg = nextResultReg;

                const std::string& lit = literals[i + 1];
                if(!lit.empty()) {
                    int litReg = allocateTempRegister();
                    code.push_back({(uint32_t)OpCode::LOAD_STR, (uint32_t)litReg, (uint32_t)internString(lit), 0});
                    int concatReg = allocateTempRegister();
                    code.push_back({(uint32_t)OpCode::ADD, (uint32_t)concatReg, (uint32_t)resultReg, (uint32_t)litReg});
                    freeTempRegister(resultReg);
                    freeTempRegister(litReg);
                    resultReg = concatReg;
                }
            }
            storage.push(resultReg);
        }
    }
    return code;
}

namespace {
    void emitStoreVariable(const std::shared_ptr<VariableNode>& var, int srcReg, std::vector<Instruction>& code) {
        if (var->getIsLocal()) {
            int32_t off = var->getLocalOffset();
            if (var->getOuterHops() > 0) {
                code.push_back({(uint32_t)OpCode::STORE_OUTER,
                    (uint32_t)srcReg,
                    (uint32_t)var->getOuterHops(),
                    (uint32_t)(uint8_t)(int8_t)off});
            } else {
                code.push_back({(uint32_t)OpCode::STORE, (uint32_t)srcReg, (uint32_t)FP, (uint32_t)off});
            }
        } else {
            code.push_back({(uint32_t)OpCode::STORE_VAR, 0, (uint32_t)var->getGlobalAddr(), (uint32_t)srcReg});
        }
    }
}

void Compiler::compileStatement(std::shared_ptr<StatementNode> stmt, std::vector<Instruction>& code) {
    if(!stmt) return;

    if (auto subWrite = std::dynamic_pointer_cast<SubscriptWriteNode>(stmt)) {
        auto var = std::dynamic_pointer_cast<VariableNode>(subWrite->getObject());

        auto valueCode = generateByteCode(postOrderTraverse(subWrite->getValue()), code.size());
        rebaseJumpTargets(valueCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), valueCode.begin(), valueCode.end());
        int valReg = valueCode.empty() ? 0 : valueCode.back().dst;

        auto indexCode = generateByteCode(postOrderTraverse(subWrite->getIndex()), code.size());
        rebaseJumpTargets(indexCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), indexCode.begin(), indexCode.end());
        int idxReg = indexCode.empty() ? 0 : indexCode.back().dst;

        int baseReg;
        if (var) {
            baseReg = allocateTempRegister();
            if (var->getIsLocal()) {
                int32_t off = var->getLocalOffset();
                if (var->getOuterHops() > 0) {
                    code.push_back({(uint32_t)OpCode::LOAD_OUTER, (uint32_t)baseReg, (uint32_t)var->getOuterHops(),
                        (uint32_t)(uint8_t)(int8_t)off});
                } else {
                    code.push_back({(uint32_t)OpCode::LOAD, (uint32_t)baseReg, (uint32_t)FP, (uint32_t)off});
                }
            } else {
                code.push_back({(uint32_t)OpCode::LOAD_VAR, (uint32_t)baseReg, (uint32_t)var->getGlobalAddr(), 0});
            }
        } else {
            auto baseCode = generateByteCode(postOrderTraverse(subWrite->getObject()), code.size());
            rebaseJumpTargets(baseCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), baseCode.begin(), baseCode.end());
            baseReg = baseCode.empty() ? 0 : baseCode.back().dst;
        }

        code.push_back({(uint32_t)OpCode::STORE_STR_IDX, (uint32_t)valReg, (uint32_t)baseReg, (uint32_t)idxReg});
        addLineNumbers(stmt->lineNumber, 1);
        if (var) {
            // Harmless (a no-op copy) for arrays; required for strings.
            emitStoreVariable(var, baseReg, code);
        }

        freeTempRegister(valReg);
        freeTempRegister(idxReg);
        freeTempRegister(baseReg);
        return;
    }
    
    if (auto assign = std::dynamic_pointer_cast<AssignmentNode>(stmt)) {
        auto exprCode = generateByteCode(postOrderTraverse(assign->getValue()), code.size());
        rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), exprCode.begin(), exprCode.end());
        addLineNumbers(stmt->lineNumber, exprCode.size());

        int srcReg = exprCode.empty() ? 0 : exprCode.back().dst;
        
        if (assign->isLocal()) {
            int32_t offset = assign->getOffset();
            int oh = assign->getLocalOuterHops();
            if (oh > 0) {
                code.push_back({(uint32_t)OpCode::STORE_OUTER,
                    (uint32_t)srcReg,
                    (uint32_t)oh,
                    (uint32_t)(uint8_t)(int8_t)offset});
            } else {
                code.push_back({(uint32_t)OpCode::STORE, 
                                (uint32_t)srcReg,
                                (uint32_t)FP,
                                (uint32_t)offset});
            }
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        } 
        else {
            size_t addr = assign->getAddress();
            code.push_back({(uint32_t)OpCode::STORE_VAR, 
                            0,
                            (uint32_t)addr,
                            (uint32_t)srcReg});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        }
        freeTempRegister(srcReg);
    }
    
    else if(auto ifStmt = std::dynamic_pointer_cast<IfStatementNode>(stmt)) {
        auto condCode = generateByteCode(postOrderTraverse(ifStmt->getCondition()), code.size());
        rebaseJumpTargets(condCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), condCode.begin(), condCode.end());
        addLineNumbers(stmt->lineNumber, condCode.size());
        int condReg = condCode.empty() ? 0 : condCode.back().dst;
        size_t jzIdx = code.size();
        code.push_back({(uint32_t)OpCode::JZ, (uint32_t)condReg, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        freeTempRegister(condReg);
        
        compileStatement(ifStmt->getThenBr(), code);
        size_t jmpIdx = code.size();
        code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        setAddress(code[jzIdx], (uint16_t)code.size());
        if(ifStmt->getElseBr()) compileStatement(ifStmt->getElseBr(), code);
        setAddress(code[jmpIdx], (uint16_t)code.size());
    }
    else if(auto whileStmt = std::dynamic_pointer_cast<WhileStatementNode>(stmt)) {
        // Push new break/continue lists for this loop
        breakStack.push({});
        continueStack.push({});
        
        size_t startAddr = code.size();
        auto condCode = generateByteCode(postOrderTraverse(whileStmt->getCondition()), code.size());
        rebaseJumpTargets(condCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), condCode.begin(), condCode.end());
        addLineNumbers(stmt -> lineNumber, condCode.size());
        int condReg = condCode.empty() ? 0 : condCode.back().dst;
        size_t jzIdx = code.size();
        code.push_back({(uint32_t)OpCode::JZ, (uint32_t)condReg, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        freeTempRegister(condReg);
        
        compileStatement(whileStmt->getBody(), code);
        
        // Jump back to condition
        Instruction jmpBack = {(uint32_t)OpCode::JMP, 0, 0, 0};
        setAddress(jmpBack, (uint16_t)startAddr);
        code.push_back(jmpBack);
        lineNumbers.push_back(stmt -> lineNumber);
        size_t afterLoopAddr = code.size();
        
        // JZ should jump after the cycle
        setAddress(code[jzIdx], (uint16_t)afterLoopAddr);
        
        // Patch all break jumps to point after the loop
        while (!breakStack.top().empty()) {
            size_t breakIdx = breakStack.top().back();
            breakStack.top().pop_back();
            setAddress(code[breakIdx], (uint16_t)afterLoopAddr);
        }
        
        // Patch all continue jumps to point to condition check (startAddr)
        while (!continueStack.top().empty()) {
            size_t continueIdx = continueStack.top().back();
            continueStack.top().pop_back();
            setAddress(code[continueIdx], (uint16_t)startAddr);
        }
        
        breakStack.pop();
        continueStack.pop();
    }
    else if(auto block = std::dynamic_pointer_cast<BlockCode>(stmt)) {
        for(auto& s : block->getStatements()) compileStatement(s, code);
    }
    else if(auto printStmt = std::dynamic_pointer_cast<PrintNode>(stmt)) {
        for(const auto& expr : printStmt->getExpressions()) {
            if(auto strNode = std::dynamic_pointer_cast<StringNode>(expr)) {
                const std::string& val = strNode->getValue();
                int strIdx;
                auto it = stringMap.find(val);
                if(it != stringMap.end()) {
                    strIdx = it->second;
                } else {
                    strIdx = (int)stringPool.size();
                    stringPool.push_back(val);
                    stringMap[val] = strIdx;
                }
                code.push_back({(uint32_t)OpCode::PRINT_STR, (uint32_t)strIdx, 0, 0});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            } else {
                auto exprCode = generateByteCode(postOrderTraverse(expr), code.size());
                rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), exprCode.begin(), exprCode.end());
                for(size_t i = 0; i < exprCode.size(); ++i) {
                    lineNumbers.push_back(stmt -> lineNumber);
                }
                int lastReg = exprCode.empty() ? 0 : exprCode.back().dst;
                code.push_back({(uint32_t)OpCode::PRINT, (uint32_t)lastReg, 0, 0});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
                freeTempRegister(lastReg);
            }
        }
    }
    else if(auto forStmt = std::dynamic_pointer_cast<ForStatementNode>(stmt)) {
        // Push new break/continue lists for this loop
        breakStack.push({});
        continueStack.push({});
        
        compileStatement(forStmt->getInit(), code);
        size_t startAddr = code.size();

        auto condCode = generateByteCode(postOrderTraverse(forStmt->getCondition()), code.size());
        rebaseJumpTargets(condCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), condCode.begin(), condCode.end());
        addLineNumbers(stmt -> lineNumber, condCode.size());
        int condReg = condCode.empty() ? 0 : condCode.back().dst;
        size_t jzIdx = code.size();
        code.push_back({(uint32_t)OpCode::JZ, (uint32_t)condReg, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        freeTempRegister(condReg);

        compileStatement(forStmt->getBody(), code);
        
        size_t updateAddr = code.size();
        
        // Patch all continue jumps to point to update statement
        while (!continueStack.top().empty()) {
            size_t continueIdx = continueStack.top().back();
            continueStack.top().pop_back();
            setAddress(code[continueIdx], (uint16_t)updateAddr);
        }

        compileStatement(forStmt->getUpdate(), code);

        // Jump back to condition
        Instruction jmpFor = {(uint32_t)OpCode::JMP, 0, 0, 0};
        setAddress(jmpFor, (uint16_t)startAddr);
        code.push_back(jmpFor);
        lineNumbers.push_back(stmt -> lineNumber);
        
        size_t afterLoopAddr = code.size();
        
        setAddress(code[jzIdx], (uint16_t)afterLoopAddr);
        
        // Patch all break jumps to point after the loop
        while (!breakStack.top().empty()) {
            size_t breakIdx = breakStack.top().back();
            breakStack.top().pop_back();
            setAddress(code[breakIdx], (uint16_t)afterLoopAddr);
        }
        
        breakStack.pop();
        continueStack.pop();
    }   
    else if (auto funcDef = std::dynamic_pointer_cast<FunctionDefNode>(stmt)) {
        const std::string& funcName = funcDef->getName();
        if (functionTable.count(funcName)) {
            throw std::runtime_error("Function redefinition is not allowed: " + funcName);
        }
        size_t jmpIdx = code.size();
        code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        size_t funcAddr = code.size();
        const auto& funcParams = funcDef->getParams();
        int namedCount = (int)funcParams.size();
        int minRequired = 0;
        for (const auto& p : funcParams) {
            if (!p.defaultValue) minRequired++;
        }
        functionTable[funcName] = {funcAddr, namedCount, minRequired, funcDef->hasVariadic()};

        int slots = funcDef->getLocalSlotCount();
        if (slots < 1) slots = 1;
        int frameSize = (slots + 4) * 4;
        code.push_back({(uint32_t)OpCode::ADDI, SP, SP, (uint32_t)(int32_t)(-frameSize)});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        code.push_back({(uint32_t)OpCode::ADDI, FP, SP, (uint32_t)frameSize});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);

        for (int i = 0; i < namedCount; i++) {
            int32_t off = -4 * (i + 1);
            const ParamInfo& p = funcParams[i];

            if (!p.defaultValue) {
                // Required parameter - call sites already guarantee it's present.
                int reg = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::LOAD_PARAM, (uint32_t)reg, (uint32_t)i, 0});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
                code.push_back({(uint32_t)OpCode::STORE, (uint32_t)reg, (uint32_t)FP, (uint32_t)off});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
                freeTempRegister(reg);
                continue;
            }

            // Optional parameter: if the caller actually passed this many
            // args, use it; otherwise evaluate the default expression.
            int argcReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::ARGC, (uint32_t)argcReg, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);

            int idxReg = allocateTempRegister();
            int idxConstIdx;
            auto cit = constMap.find((double)i);
            if (cit != constMap.end()) idxConstIdx = cit->second;
            else {
                idxConstIdx = (int)constantPool.size();
                constantPool.push_back((double)i);
                constMap[(double)i] = idxConstIdx;
            }
            code.push_back({(uint32_t)OpCode::LOAD_CONST, (uint32_t)idxReg, (uint32_t)idxConstIdx, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);

            int hasArgReg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::CMP_LT, (uint32_t)hasArgReg, (uint32_t)idxReg, (uint32_t)argcReg});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(idxReg);
            freeTempRegister(argcReg);

            size_t jzIdx = code.size();
            code.push_back({(uint32_t)OpCode::JZ, (uint32_t)hasArgReg, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(hasArgReg);

            // Caller passed this arg -> take it via LOAD_PARAM.
            int reg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::LOAD_PARAM, (uint32_t)reg, (uint32_t)i, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            code.push_back({(uint32_t)OpCode::STORE, (uint32_t)reg, (uint32_t)FP, (uint32_t)off});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(reg);

            size_t jmpIdx = code.size();
            code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            setAddress(code[jzIdx], (uint16_t)code.size());

            // Caller omitted this arg -> evaluate the default expression.
            auto defCode = generateByteCode(postOrderTraverse(p.defaultValue), code.size());
            rebaseJumpTargets(defCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), defCode.begin(), defCode.end());
            addLineNumbers(stmt ? stmt -> lineNumber : 0, defCode.size());
            int defReg = defCode.empty() ? 0 : defCode.back().dst;
            code.push_back({(uint32_t)OpCode::STORE, (uint32_t)defReg, (uint32_t)FP, (uint32_t)off});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(defReg);

            setAddress(code[jmpIdx], (uint16_t)code.size());
        }

        if (funcDef->hasVariadic()) {
            int32_t off = -4 * (namedCount + 1);
            int reg = allocateTempRegister();
            code.push_back({(uint32_t)OpCode::COLLECT_VARARGS, (uint32_t)reg, (uint32_t)namedCount, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            code.push_back({(uint32_t)OpCode::STORE, (uint32_t)reg, (uint32_t)FP, (uint32_t)off});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(reg);
        }

        compileStatement(funcDef->getBody(), code);
        if (funcDef->getIsVoid()) {
            code.push_back({(uint32_t)OpCode::RETURN, 0, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        }
        setAddress(code[jmpIdx], (uint16_t)code.size());
    } 
    else if(auto callStmt = std::dynamic_pointer_cast<FunctionCallStatementNode>(stmt)) {
        auto call = callStmt->getCall();
        int builtinResultReg = 0;
        size_t instCountBefore = code.size();
        if(tryEmitMathBuiltinCall(call->getName(), call->getArgs(), code, builtinResultReg, code.size())) {
            addLineNumbers(callStmt->lineNumber, code.size() - instCountBefore);
            freeTempRegister(builtinResultReg);
            return;
        }
        
        // Argument count check
        auto it = functionTable.find(call -> getName());
        if(it != functionTable.end()) {
            checkCallArgCount(call->getName(), it->second, call->getArgs().size());
        }

        for(const auto& arg : call->getArgs()) {
            auto exprCode = generateByteCode(postOrderTraverse(arg), code.size());
            rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), exprCode.begin(), exprCode.end());
            addLineNumbers(stmt->lineNumber, exprCode.size());
            int argReg = exprCode.empty() ? 0 : exprCode.back().dst;
            code.push_back({(uint32_t)OpCode::PUSH_ARG, (uint32_t)argReg, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            freeTempRegister(argReg);
        }
        int resultReg = allocateTempRegister();
        Instruction callInst;
        callInst.op  = (uint32_t)OpCode::CALL;
        callInst.dst = (uint32_t)resultReg;
        if(functionTable.count(call->getName())) {
            setAddress(callInst, (uint16_t)functionTable[call->getName()].address);
        } else {
            setAddress(callInst, 0);
            forwardCalls.push_back({code.size(), call->getName()});
        }
        code.push_back(callInst);
        lineNumbers.push_back(stmt -> lineNumber);
        
        freeTempRegister(resultReg);
    }
    else if(auto retStmt = std::dynamic_pointer_cast<ReturnNode>(stmt)) {
        if(retStmt->getExpression()) {
            auto exprCode = generateByteCode(postOrderTraverse(retStmt->getExpression()), code.size());
            rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
            code.insert(code.end(), exprCode.begin(), exprCode.end());
            addLineNumbers(stmt -> lineNumber, exprCode.size());
            int lastReg = exprCode.empty() ? 0 : exprCode.back().dst;
            code.push_back({(uint32_t)OpCode::RETURN, (uint32_t)lastReg, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        } else {
            code.push_back({(uint32_t)OpCode::RETURN, 0, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        }
    } else if(auto breakStmt = std::dynamic_pointer_cast<BreakNode>(stmt)) {
        if(breakStack.empty()) {
            throw std::runtime_error("break outside of loop or switch");
        }
        size_t jmpIdx = code.size();
        code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        breakStack.top().push_back(jmpIdx);
    } else if(auto continueStmt = std::dynamic_pointer_cast<ContinueNode>(stmt)) {
        if(continueStack.empty()) {
            throw std::runtime_error("continue outside of loop");
        }
        size_t jmpIdx = code.size();
        code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
        lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
        continueStack.top().push_back(jmpIdx);
    } else if(auto switchNode = std::dynamic_pointer_cast<SwitchNode>(stmt)) {
        breakStack.push({});  // for breaks
        
        // 1. Switch expression
        auto exprCode = generateByteCode(postOrderTraverse(switchNode->getExpression()), code.size());
        rebaseJumpTargets(exprCode, static_cast<uint16_t>(code.size()));
        code.insert(code.end(), exprCode.begin(), exprCode.end());
        addLineNumbers(stmt -> lineNumber, exprCode.size());
        int switchValReg = exprCode.empty() ? 0 : exprCode.back().dst;
        
        const auto& cases = switchNode->getCases();
        size_t numCases = cases.size();
        bool hasDefault = (switchNode->getDefaultBody() != nullptr);
        
        std::vector<size_t> caseCheckStart(numCases);
        
        std::vector<std::vector<size_t>> caseJumpTargets(numCases);
        
        std::vector<size_t> nextCheckJumps;
        
        // 2. Checking all cases' values
        for(size_t i = 0; i < numCases; ++i) {
            caseCheckStart[i] = code.size();
        
            const auto& caseItem = cases[i];
            for(const auto& valueExpr : caseItem.values) {
                auto valCode = generateByteCode(postOrderTraverse(valueExpr), code.size());
                rebaseJumpTargets(valCode, static_cast<uint16_t>(code.size()));
                code.insert(code.end(), valCode.begin(), valCode.end());
                addLineNumbers(stmt -> lineNumber, valCode.size());
                int valReg = valCode.empty() ? 0 : valCode.back().dst;
            
                // compare switchValReg == valReg
                int cmpReg = allocateTempRegister();
                code.push_back({(uint32_t)OpCode::CMP_EQ, (uint32_t)cmpReg, (uint32_t)switchValReg, (uint32_t)valReg});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            
                // (JZ false -> jump if zero)
                size_t jzIdx = code.size();
                code.push_back({(uint32_t)OpCode::JZ, (uint32_t)cmpReg, 0, 0});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
                // if true jump to case-body
                size_t jmpToBodyIdx = code.size();
                code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
                lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            
                setAddress(code[jzIdx], (uint16_t)code.size());
            
                caseJumpTargets[i].push_back(jmpToBodyIdx);
            
                freeTempRegister(valReg);
                freeTempRegister(cmpReg);
            }
        
            size_t jmpNextIdx = code.size();
            code.push_back({(uint32_t)OpCode::JMP, 0, 0, 0});
            lineNumbers.push_back(stmt ? stmt -> lineNumber : 0);
            nextCheckJumps.push_back(jmpNextIdx);
        }
    
        // 3. Cases' bodies
        std::vector<size_t> bodyAddrs(numCases);
        for(size_t i = 0; i < numCases; ++i) {
            bodyAddrs[i] = code.size();
            
            for(size_t jmpIdx : caseJumpTargets[i]) {
                setAddress(code[jmpIdx], (uint16_t)bodyAddrs[i]);
            }
            compileStatement(cases[i].body, code);
        }
    
        // 4. default body (if there's)
        size_t defaultAddr = code.size();
        if(hasDefault) {
            compileStatement(switchNode->getDefaultBody(), code);
        }
        size_t endAddr = code.size();
    
        // 5. Patch nextCheckJumps
        for(size_t i = 0; i < numCases; ++i) {
            size_t target;
            if(i + 1 < numCases) {
                target = caseCheckStart[i+1];
            } else {
                target = hasDefault ? defaultAddr : endAddr;
            }
            setAddress(code[nextCheckJumps[i]], (uint16_t)target);
        }
    
        // 6. Patch breaks at the end of switch
        while(!breakStack.top().empty()) {
            size_t brkIdx = breakStack.top().back();
            breakStack.top().pop_back();
            setAddress(code[brkIdx], (uint16_t)endAddr);
        }
        breakStack.pop();
    
        freeTempRegister(switchValReg);
    } 
}

void writeByteCodeToFile(const ByteCode& bc, const std::string& path) {
    std::ofstream out(path, std::ios::binary);
    if(!out.is_open()) {
        throw std::runtime_error("Cannot open output file: " + path);
    }

    const char magic[4] = {'V', 'H', 'B', '1'};
    out.write(magic, sizeof(magic));

    uint32_t instructionCount = static_cast<uint32_t>(bc.instructions.size());
    uint32_t constantCount = static_cast<uint32_t>(bc.constants.size());
    uint32_t stringCount = static_cast<uint32_t>(bc.strings.size());

    out.write(reinterpret_cast<const char*>(&instructionCount), sizeof(instructionCount));
    out.write(reinterpret_cast<const char*>(&constantCount), sizeof(constantCount));
    out.write(reinterpret_cast<const char*>(&stringCount), sizeof(stringCount));

    uint32_t lineCount = static_cast<uint32_t>(bc.lineNumbers.size());
    out.write(reinterpret_cast<const char*>(&lineCount), sizeof(lineCount));
    for(int line : bc.lineNumbers) {
        uint32_t l = static_cast<uint32_t>(line);
        out.write(reinterpret_cast<const char*>(&l), sizeof(l));
    }

    for(const auto& inst : bc.instructions) {
        uint8_t op = static_cast<uint8_t>(inst.op);
        uint8_t dst = static_cast<uint8_t>(inst.dst);
        uint8_t left = static_cast<uint8_t>(inst.left);
        uint8_t right = static_cast<uint8_t>(inst.right);
        out.write(reinterpret_cast<const char*>(&op), sizeof(op));
        out.write(reinterpret_cast<const char*>(&dst), sizeof(dst));
        out.write(reinterpret_cast<const char*>(&left), sizeof(left));
        out.write(reinterpret_cast<const char*>(&right), sizeof(right));
    }

    for(double value : bc.constants) {
        out.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }

    for(const auto& str : bc.strings) {
        uint32_t len = static_cast<uint32_t>(str.size());
        out.write(reinterpret_cast<const char*>(&len), sizeof(len));
        out.write(str.data(), len);
    }

    uint32_t gsc = static_cast<uint32_t>(bc.globalSlotCount);
    out.write(reinterpret_cast<const char*>(&gsc), sizeof(gsc));
    for (uint32_t i = 0; i < gsc; ++i) {
        const std::string& nm = (i < bc.globalNamesBySlot.size()) ? bc.globalNamesBySlot[i] : std::string{};
        uint32_t nlen = static_cast<uint32_t>(nm.size());
        out.write(reinterpret_cast<const char*>(&nlen), sizeof(nlen));
        if (nlen) out.write(nm.data(), nlen);
    }

    uint32_t funcSymCount = static_cast<uint32_t>(bc.functionSymbols.size());
    out.write(reinterpret_cast<const char*>(&funcSymCount), sizeof(funcSymCount));
    for (const auto& [name, addr] : bc.functionSymbols) {
        uint32_t nlen = static_cast<uint32_t>(name.size());
        out.write(reinterpret_cast<const char*>(&nlen), sizeof(nlen));
        if (nlen) out.write(name.data(), nlen);
        uint32_t absAddr = static_cast<uint32_t>(addr);
        out.write(reinterpret_cast<const char*>(&absAddr), sizeof(absAddr));
    }

    uint32_t unresolvedCount = static_cast<uint32_t>(bc.unresolvedCalls.size());
    out.write(reinterpret_cast<const char*>(&unresolvedCount), sizeof(unresolvedCount));
    for (const auto& [localIdx, funcName] : bc.unresolvedCalls) {
        uint32_t idx = static_cast<uint32_t>(localIdx);
        out.write(reinterpret_cast<const char*>(&idx), sizeof(idx));
        uint32_t nlen = static_cast<uint32_t>(funcName.size());
        out.write(reinterpret_cast<const char*>(&nlen), sizeof(nlen));
        if (nlen) out.write(funcName.data(), nlen);
    }

    if(!out.good()) {
        throw std::runtime_error("Failed writing bytecode file: " + path);
    }
}

ByteCode readByteCodeFromFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if(!in.is_open()) {
        throw std::runtime_error("Cannot open bytecode file: " + path);
    }

    char magic[4] = {};
    in.read(magic, sizeof(magic));
    const char expected[4] = {'V', 'H', 'B', '1'};
    if(std::memcmp(magic, expected, sizeof(expected)) != 0) {
        throw std::runtime_error("Invalid bytecode format: " + path);
    }

    uint32_t instructionCount = 0;
    uint32_t constantCount = 0;
    uint32_t stringCount = 0;
    in.read(reinterpret_cast<char*>(&instructionCount), sizeof(instructionCount));
    in.read(reinterpret_cast<char*>(&constantCount), sizeof(constantCount));
    in.read(reinterpret_cast<char*>(&stringCount), sizeof(stringCount));

    ByteCode bc;
    bc.instructions.reserve(instructionCount);
    bc.constants.resize(constantCount);
    bc.strings.reserve(stringCount);

    uint32_t lineCount = 0;
    in.read(reinterpret_cast<char*>(&lineCount), sizeof(lineCount));
    bc.lineNumbers.resize(lineCount);
    for(uint32_t i = 0; i < lineCount; ++i) {
        uint32_t l = 0;
        in.read(reinterpret_cast<char*>(&l), sizeof(l));
        bc.lineNumbers[i] = static_cast<int>(l);
    }

    for(uint32_t i = 0; i < instructionCount; ++i) {
        uint8_t op = 0, dst = 0, left = 0, right = 0;
        in.read(reinterpret_cast<char*>(&op), sizeof(op));
        in.read(reinterpret_cast<char*>(&dst), sizeof(dst));
        in.read(reinterpret_cast<char*>(&left), sizeof(left));
        in.read(reinterpret_cast<char*>(&right), sizeof(right));
        bc.instructions.push_back({op, dst, left, right});
    }

    for(uint32_t i = 0; i < constantCount; ++i) {
        in.read(reinterpret_cast<char*>(&bc.constants[i]), sizeof(double));
    }

    for(uint32_t i = 0; i < stringCount; ++i) {
        uint32_t len = 0;
        in.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, '\0');
        if(len > 0) {
            in.read(&s[0], len);
        }
        bc.strings.push_back(std::move(s));
    }

    bc.globalSlotCount = 0;
    bc.globalNamesBySlot.clear();
    {
        std::streampos pos = in.tellg();
        uint32_t gsc = 0;
        in.read(reinterpret_cast<char*>(&gsc), sizeof(gsc));
        if (in.gcount() != static_cast<std::streamsize>(sizeof(gsc))) {
            in.clear();
            in.seekg(pos);
        } else {
            bc.globalSlotCount = gsc;
            bc.globalNamesBySlot.resize(gsc);
            bool ok = true;
            for (uint32_t i = 0; ok && i < gsc; ++i) {
                uint32_t nlen = 0;
                in.read(reinterpret_cast<char*>(&nlen), sizeof(nlen));
                if (in.gcount() != static_cast<std::streamsize>(sizeof(nlen))) {
                    ok = false;
                    break;
                }
                std::string s(nlen, '\0');
                if (nlen > 0) {
                    in.read(&s[0], nlen);
                    if (!in) ok = false;
                }
                if (ok) bc.globalNamesBySlot[i] = std::move(s);
            }
            if (!ok) {
                bc.globalSlotCount = 0;
                bc.globalNamesBySlot.clear();
                in.clear();
                in.seekg(pos);
            }
        }
    }

    auto readOptionalSection = [&](auto readFn) {
        std::streampos pos = in.tellg();
        if (!in) return;
        if (!readFn()) {
            in.clear();
            in.seekg(pos);
        }
    };

    readOptionalSection([&]() -> bool {
        uint32_t funcSymCount = 0;
        in.read(reinterpret_cast<char*>(&funcSymCount), sizeof(funcSymCount));
        if (in.gcount() != static_cast<std::streamsize>(sizeof(funcSymCount))) return false;
        bc.functionSymbols.clear();
        for (uint32_t i = 0; i < funcSymCount; ++i) {
            uint32_t nlen = 0;
            in.read(reinterpret_cast<char*>(&nlen), sizeof(nlen));
            if (in.gcount() != static_cast<std::streamsize>(sizeof(nlen))) return false;
            std::string name(nlen, '\0');
            if (nlen > 0) {
                in.read(name.data(), nlen);
                if (!in) return false;
            }
            uint32_t addr = 0;
            in.read(reinterpret_cast<char*>(&addr), sizeof(addr));
            if (in.gcount() != static_cast<std::streamsize>(sizeof(addr))) return false;
            bc.functionSymbols[std::move(name)] = addr;
        }
        return true;
    });

    readOptionalSection([&]() -> bool {
        uint32_t unresolvedCount = 0;
        in.read(reinterpret_cast<char*>(&unresolvedCount), sizeof(unresolvedCount));
        if (in.gcount() != static_cast<std::streamsize>(sizeof(unresolvedCount))) return false;
        bc.unresolvedCalls.clear();
        bc.unresolvedCalls.reserve(unresolvedCount);
        for (uint32_t i = 0; i < unresolvedCount; ++i) {
            uint32_t localIdx = 0;
            in.read(reinterpret_cast<char*>(&localIdx), sizeof(localIdx));
            if (in.gcount() != static_cast<std::streamsize>(sizeof(localIdx))) return false;
            uint32_t nlen = 0;
            in.read(reinterpret_cast<char*>(&nlen), sizeof(nlen));
            if (in.gcount() != static_cast<std::streamsize>(sizeof(nlen))) return false;
            std::string funcName(nlen, '\0');
            if (nlen > 0) {
                in.read(funcName.data(), nlen);
                if (!in) return false;
            }
            bc.unresolvedCalls.emplace_back(localIdx, std::move(funcName));
        }
        return true;
    });

    if(!in.good() && !in.eof()) {
        throw std::runtime_error("Failed reading bytecode file: " + path);
    }
    return bc;
}