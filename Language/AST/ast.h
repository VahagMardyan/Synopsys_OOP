#pragma once
#include <memory>
#include <map>
#include <string>
#include <cstdint>
#include <vector>
#include "../SymbolTable/symbol_table.h"

enum class OpCode : uint8_t {
    // RV32I arithmetic and logic
    ADD, MOV, SUB, AND, OR, XOR, NOT, // bitwise not
    SLL, SRL, SRA,
    SLT, SLTU,
    ADDI, ANDI, ORI, XORI,
    SLLI, SRLI, SRAI,
    LUI, AUIPC,

    // RV32I control flow
    JAL, JALR,
    BEQ, BNE, BLT, BGE, BLTU, BGEU,

    // RV32I memory
    LW, SW,

    // Existing VM extensions
    MUL, DIV, MODULO, POW, FLOOR_DIV, FRAC_DIV,
    UNARY, LOAD_CONST, LOAD_VAR, LOAD_STR, LOAD_NONE, 
    UNDEFINED,
    CMP_GT, CMP_LT, CMP_GET, CMP_LET, CMP_EQ, CMP_NEQ,
    JMP, JZ, JNZ,
    STORE_VAR,
    PRINT, PRINT_STR,
    LOGICAL_AND, LOGICAL_OR, LOGICAL_NOT,
    CALL, RETURN, PUSH_ARG, LOAD_PARAM,
    LOAD, STORE,
    INPUT, // user-input
    LENGTH, // string length
    // math functions
    SIN, COS, TAN,
    ASIN, ACOS, ATAN, ATAN2,
    SQRT, EXP, LOG, LOG10,
    CEIL, FLOOR, ABS, ROUND,
    FMOD, CBRT, MATH_POW, LOG2, LOG_AB, // log(b)/log(a)
    // math constants
    CONST_PI, CONST_E, CONST_INF, CONST_MAX, // pi, e, infinity, max
    // Conversions
    ORD, // char -> int
    CHR, // int -> char
    BIN, // int -> binary string
    OCT, // int -> octal string
    DEC, // int -> decimal string
    HEX, // int -> hexadecimal string
    TYPE, // type(argument) -> "string" / "number" / "none"
    TO_NUMBER, // number(argument) -> parses/coerces argument into a number
    TO_STRING, // string(argument) -> renders argument as a string (same text print() shows)
    LOAD_OUTER, // dst = mem[enclosingCallerFp(hops) + int8_offset]
    STORE_OUTER, // mem[enclosingCallerFp(hops) + int8_offset] = dstReg
    RANDOM, // random(min=0, max=1)
    LOAD_STR_IDX, // dst = base[left][right] - generic subscript-read; base may be a
                  // string (single-char result) or an array (element result)
    STORE_STR_IDX, // base[left][right] = dst - generic subscript-write; base may be a
                   // string (mutates local copy, caller re-stores it) or an array
                   // (mutates the shared underlying storage in place)
    // Arrays
    ARRAY_NEW,    // dst = new array of size asNumber(reg[left]), filled with none
    ARRAY_LIT,    // dst = new empty array (elements are appended one at a time
                  // via ARRAY_PUSH right after this, in source order)
    ARRAY_PUSH,   // dst = new length; appends reg[right] to array reg[left]
    ARRAY_POP,    // dst = removed last element of array reg[left]
    ARRAY_INSERT, // array reg[left].insert(idx=reg[right], value=reg[dst])
    ARRAY_REMOVE, // dst = removed element at idx=reg[right] from array reg[left]

    // Default arguments / variadic (*args) support
    ARGC,             // dst = number of args actually passed to the current call frame
    COLLECT_VARARGS,  // dst = the variadic args as an array. Normally a new array
                      // built from callFrame.args[left .. end); but if exactly one
                      // variadic argument was passed and it is itself an array, that
                      // array is used directly (unwrapped) - this is what makes
                      // sum([1,2,3]) behave like sum(1,2,3) for a `*args` parameter.
};

class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual std::vector<std::shared_ptr<ASTNode>> getChildren() const = 0;
};

class NumberNode : public ASTNode {
    double value;
public:
    NumberNode(double val) : value(val) {}
    double getValue() const { return value; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class MathConstantNode : public ASTNode {
    private:
        OpCode constant;
    public:
        MathConstantNode(OpCode c) : constant(c) {}
        OpCode getConstant() const { return constant; }
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class VariableNode : public ASTNode {
    bool isLocal;
    int outerHops_ = 0; // 0 = current frame; N>=1 = walk N callerFp steps for lexical outer
    union {
        size_t globalAddr;
        int32_t localOffset;
    };
public:
    VariableNode(size_t addr) : isLocal(false), outerHops_(0), globalAddr(addr) {}
    VariableNode(int32_t off)  : isLocal(true), outerHops_(0), localOffset(off) {}
    VariableNode(int32_t off, int outerHops)
        : isLocal(true), outerHops_(outerHops), localOffset(off) {}

    bool getIsLocal() const { return isLocal; }
    int getOuterHops() const { return outerHops_; }
    size_t getGlobalAddr() const { return globalAddr; }
    int32_t getLocalOffset() const { return localOffset; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class BinaryOpNode : public ASTNode {
    std::string op;
    std::shared_ptr<ASTNode> left, right;
public:
    BinaryOpNode(const std::string& o, std::shared_ptr<ASTNode> l, std::shared_ptr<ASTNode> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    OpCode getOpCode() const;
    std::shared_ptr<ASTNode> getLeft() const { return left; }
    std::shared_ptr<ASTNode> getRight() const { return right; }
    std::string getOp() const { return op; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {left, right}; }
};

class UnaryOpNode : public ASTNode {
    std::string op;
    std::shared_ptr<ASTNode> child;
public:
    UnaryOpNode(const std::string& o, std::shared_ptr<ASTNode> c) : op(o), child(std::move(c)) {}
    std::string getOp() const { return op; }
    std::shared_ptr<ASTNode> getChild() const { return child; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {child}; }
};

class TernaryOpNode : public ASTNode {
    private:
        std::shared_ptr<ASTNode> condition;
        std::shared_ptr<ASTNode> trueExpr;
        std::shared_ptr<ASTNode> falseExpr;
    public:
        TernaryOpNode(std::shared_ptr<ASTNode> cond, std::shared_ptr<ASTNode> trueExp, std::shared_ptr<ASTNode> falseExp)
            : condition(std::move(cond)), trueExpr(std::move(trueExp)), falseExpr(std::move(falseExp)) {}

        std::shared_ptr<ASTNode> getCondition() const { return condition; }
        std::shared_ptr<ASTNode> getTrueExpr() const { return trueExpr; }
        std::shared_ptr<ASTNode> getFalseExpr() const { return falseExpr; }       
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override {
            return {condition, trueExpr, falseExpr};
        }
};

class NoneNode : public ASTNode {
    public:
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// Array literal: [e1, e2, ...] (elements may themselves be array literals,
// giving matrices / nested arrays). Elements are intentionally NOT exposed
// via getChildren() (mirroring FunctionCallNode) - the compiler walks
// getElements() itself so it can stage each value through PUSH_ARG before
// emitting ARRAY_LIT, the same mechanism used for function-call arguments.
class ArrayLiteralNode : public ASTNode {
    std::vector<std::shared_ptr<ASTNode>> elements;
public:
    ArrayLiteralNode(std::vector<std::shared_ptr<ASTNode>> elems) : elements(std::move(elems)) {}
    const std::vector<std::shared_ptr<ASTNode>>& getElements() const { return elements; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class StatementNode : public ASTNode {
    public:
        int lineNumber = 0;
        virtual ~StatementNode() = default;
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class SubscriptReadNode : public ASTNode {
    std::shared_ptr<ASTNode> object;
    std::shared_ptr<ASTNode> index;
public:
    SubscriptReadNode(std::shared_ptr<ASTNode> obj, std::shared_ptr<ASTNode> idx)
        : object(std::move(obj)), index(std::move(idx)) {}
    std::shared_ptr<ASTNode> getObject() const { return object; }
    std::shared_ptr<ASTNode> getIndex() const { return index; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {object, index}; }
};

class SubscriptWriteNode : public StatementNode {
    std::shared_ptr<ASTNode> object;
    std::shared_ptr<ASTNode> index;
    std::shared_ptr<ASTNode> value;
public:
    SubscriptWriteNode(std::shared_ptr<ASTNode> obj, std::shared_ptr<ASTNode> idx,
                       std::shared_ptr<ASTNode> val)
        : object(std::move(obj)), index(std::move(idx)), value(std::move(val)) {}
    std::shared_ptr<ASTNode> getObject() const { return object; }
    std::shared_ptr<ASTNode> getIndex() const { return index; }
    std::shared_ptr<ASTNode> getValue() const { return value; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {object, index, value}; }
};

class BlockCode : public StatementNode {
    std::vector<std::shared_ptr<StatementNode>> statements;
public:
    void addStatement(std::shared_ptr<StatementNode> stmt) { statements.push_back(stmt); }
    std::vector<std::shared_ptr<StatementNode>> getStatements() const { return statements; }
};

class AssignmentNode : public StatementNode {
private:
    bool isLocalFlag;
    int32_t localOffset;
    int localOuterHops_ = 0;
    size_t globalAddr;
    std::shared_ptr<ASTNode> value;

public:
    AssignmentNode(int32_t offset, std::shared_ptr<ASTNode> val, int localOuterHops = 0)
        : isLocalFlag(true), localOffset(offset), localOuterHops_(localOuterHops), globalAddr(0), value(val) {}

    AssignmentNode(size_t addr, std::shared_ptr<ASTNode> val)
        : isLocalFlag(false), localOffset(0), localOuterHops_(0), globalAddr(addr), value(val) {}

    bool isLocal() const { return isLocalFlag; }
    int32_t getOffset() const { return localOffset; }
    int getLocalOuterHops() const { return localOuterHops_; }
    size_t getAddress() const { return globalAddr; }
    std::shared_ptr<ASTNode> getValue() const { return value; }
};

class IfStatementNode : public StatementNode {
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<StatementNode> thenBranch, elseBranch;
public:
    IfStatementNode(std::shared_ptr<ASTNode> cond,
                    std::shared_ptr<StatementNode> thenBr,
                    std::shared_ptr<StatementNode> elseBr = nullptr)
        : condition(cond), thenBranch(thenBr), elseBranch(elseBr) {}
    std::shared_ptr<ASTNode> getCondition() const { return condition; }
    std::shared_ptr<StatementNode> getThenBr() const { return thenBranch; }
    std::shared_ptr<StatementNode> getElseBr() const { return elseBranch; }
};

class WhileStatementNode : public StatementNode {
    std::shared_ptr<ASTNode> condition;
    std::shared_ptr<StatementNode> body;
public:
    WhileStatementNode(std::shared_ptr<ASTNode> cond, std::shared_ptr<StatementNode> b)
        : condition(std::move(cond)), body(std::move(b)) {}
    std::shared_ptr<ASTNode> getCondition() const { return condition; }
    std::shared_ptr<StatementNode> getBody() const { return body; }
};

class PrintNode : public StatementNode {
    std::vector<std::shared_ptr<ASTNode>> expressions;
public:
    PrintNode(std::vector<std::shared_ptr<ASTNode>> exprs) : expressions(std::move(exprs)) {}
    const std::vector<std::shared_ptr<ASTNode>>& getExpressions() const { return expressions; }
};

class ForStatementNode : public StatementNode {
    private:
        std::shared_ptr<StatementNode> init; // i = start
        std::shared_ptr<ASTNode> condition; // i < 10
        std::shared_ptr<StatementNode> update; // i = i+1
        std::shared_ptr<StatementNode> body; // i = i+1
    public:
        ForStatementNode(std::shared_ptr<StatementNode> in, 
                         std::shared_ptr<ASTNode> cond, 
                         std::shared_ptr<StatementNode> updt,
                         std::shared_ptr<StatementNode> bdy)
        : init(std::move(in)), condition(std::move(cond)), update(std::move(updt)), body(std::move(bdy)) {}
        std::shared_ptr<StatementNode> getInit()      const { return init; }
    std::shared_ptr<ASTNode>       getCondition() const { return condition; }
    std::shared_ptr<StatementNode> getUpdate()    const { return update; }
    std::shared_ptr<StatementNode> getBody()      const { return body; }
};

class StringNode : public ASTNode {
    private:
        std::string value;
    public:
        StringNode(const std::string& val = "") : value(val) {}
        const std::string getValue() const {
            return value;
        }
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// One function parameter: a name, and (for non-variadic params) an optional
// default-value expression evaluated at call time when the caller didn't
// supply that argument.
struct ParamInfo {
    std::string name;
    std::shared_ptr<ASTNode> defaultValue; // nullptr => required, no default
};

// function definition
class FunctionDefNode : public StatementNode {
    private:
        std::string name;
        std::vector<ParamInfo> params;
        std::string variadicName; // empty => no *args parameter; else its name
        std::shared_ptr<StatementNode> body;
        int localSlotCount;
        bool isVoid;
    public:
        FunctionDefNode(const std::string& n, std::vector<ParamInfo> p,
                        std::string variadic,
                        std::shared_ptr<StatementNode> b, int slots, bool v = false)
        : name(n), params(std::move(p)), variadicName(std::move(variadic)),
          body(std::move(b)), localSlotCount(slots), isVoid(v) {}
        const std::string& getName() const { return name; }
        const std::vector<ParamInfo>& getParams() const { return params; }
        const std::string& getVariadicName() const { return variadicName; }
        bool hasVariadic() const { return !variadicName.empty(); }
        std::shared_ptr<StatementNode> getBody() const { return body; }
        int getLocalSlotCount() const { return localSlotCount; }
        bool getIsVoid() const { return isVoid; }
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// function call
class FunctionCallNode : public ASTNode {
    private:
        std::string name;
        std::vector<std::shared_ptr<ASTNode>> args;
    public:
        FunctionCallNode(const std::string& n, std::vector<std::shared_ptr<ASTNode>> a)
        : name(n), args(std::move(a)) {}
        const std::string& getName() const { return name; }
        const std::vector<std::shared_ptr<ASTNode>>& getArgs() const { return args; }
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// return statement
class ReturnNode : public StatementNode {
    private:
        std::shared_ptr<ASTNode> expression;
    public:
        ReturnNode(std::shared_ptr<ASTNode> expr) : expression(std::move(expr)) {}
        std::shared_ptr<ASTNode> getExpression() const { return expression; }
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

class FunctionCallStatementNode : public StatementNode {
    std::shared_ptr<FunctionCallNode> call;
public:
    FunctionCallStatementNode(std::shared_ptr<ASTNode> c)
        : call(std::dynamic_pointer_cast<FunctionCallNode>(c)) {}
    std::shared_ptr<FunctionCallNode> getCall() const { return call; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// break Statement
class BreakNode : public StatementNode {
    public:
        std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

// continue Statement
class ContinueNode : public StatementNode {
public:
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};

struct CaseItem {
    std::vector<std::shared_ptr<ASTNode>> values; // case 1,...
    std::shared_ptr<StatementNode> body;
};

class SwitchNode : public StatementNode {
private:
    std::shared_ptr<ASTNode> expression;
    std::vector<CaseItem> cases;
    std::shared_ptr<StatementNode> defaultBody;   // nullptr if not
public:
    SwitchNode(std::shared_ptr<ASTNode> expr,
               std::vector<CaseItem> cs,
               std::shared_ptr<StatementNode> def)
        : expression(std::move(expr)), cases(std::move(cs)), defaultBody(std::move(def)) {}

    std::shared_ptr<ASTNode> getExpression() const { return expression; }
    const std::vector<CaseItem>& getCases() const { return cases; }
    std::shared_ptr<StatementNode> getDefaultBody() const { return defaultBody; }
    std::vector<std::shared_ptr<ASTNode>> getChildren() const override { return {}; }
};