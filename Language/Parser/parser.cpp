#include "parser.h"
#include <cmath>
#include <sstream>

std::shared_ptr<ASTNode> Parser::createBinaryNode(const std::string& op,
    std::shared_ptr<ASTNode> left, std::shared_ptr<ASTNode> right) {
    auto leftNum  = std::dynamic_pointer_cast<NumberNode>(left);
    auto rightNum = std::dynamic_pointer_cast<NumberNode>(right);
    if(leftNum && rightNum) {
        double l = leftNum->getValue(), r = rightNum->getValue();
        if(op == "+") return std::make_shared<NumberNode>(l + r);
        if(op == "-") return std::make_shared<NumberNode>(l - r);
        if(op == "*") return std::make_shared<NumberNode>(l * r);
        if(op == "**") return std::make_shared<NumberNode>(std::pow(l,r));
        if(op == "/") {
            if(r == 0) return std::make_shared<BinaryOpNode>(op, left, right);
            return std::make_shared<NumberNode>(l / r);
        }
        if(op == "//") {
            if(r == 0) return std::make_shared<BinaryOpNode>(op, left, right);
            return std::make_shared<NumberNode>(std::floor(l/r));
        }
        long long li = (long long)l, ri = (long long)r;
        if(op == "&")  return std::make_shared<NumberNode>((double)(li & ri));
        if(op == "|")  return std::make_shared<NumberNode>((double)(li | ri));
        if(op == "^")  return std::make_shared<NumberNode>((double)(li ^ ri));
        if(op == "<<") return std::make_shared<NumberNode>((double)(li << ri));
        if(op == ">>") return std::make_shared<NumberNode>((double)((uint32_t)li >> (ri & 0x1F)));
        if(op == "%")  {
            if(ri == 0) return std::make_shared<BinaryOpNode>(op, left, right);
            return std::make_shared<NumberNode>((double)(li % ri));
        }
        if(op == "%/") {
            if(r == 0) return std::make_shared<BinaryOpNode>(op, left, right);
            return std::make_shared<NumberNode>(l / r - std::floor(l / r));
        }
        if(op == "and") {
            return std::make_shared<NumberNode>((l != 0 && r != 0) ? 1.0 : 0.0);
        }
        if(op == "or") {
            return std::make_shared<NumberNode>((l != 0 || r != 0) ? 1.0 : 0.0);
        }
        if (op == ">")  return std::make_shared<NumberNode>(l > r ? 1.0 : 0.0);
        if (op == "<")  return std::make_shared<NumberNode>(l < r ? 1.0 : 0.0);
        if (op == ">=") return std::make_shared<NumberNode>(l >= r ? 1.0 : 0.0);
        if (op == "<=") return std::make_shared<NumberNode>(l <= r ? 1.0 : 0.0);
        if (op == "==") return std::make_shared<NumberNode>(l == r ? 1.0 : 0.0);
        if (op == "!=") return std::make_shared<NumberNode>(l != r ? 1.0 : 0.0);
    }
    return std::make_shared<BinaryOpNode>(op, left, right);
}

std::shared_ptr<ASTNode> Parser::resolveVariableNode(const std::string& name) {
    int32_t localOffset = 0;
    int outerHops = 0;
    if (symTable.tryResolveLocal(name, localOffset, outerHops)) {
        return std::make_shared<VariableNode>(localOffset, outerHops);
    }

    size_t globalAddr = 0;
    if (symTable.tryGetGlobalAddress(name, globalAddr)) {
        return std::make_shared<VariableNode>(globalAddr);
    }

    error("Undefined variable: " + name);
    return nullptr;
}

bool Parser::shouldDefaultToLocal(bool explicitGlobal) const {
    // return !explicitGlobal && symTable.hasActiveScope();
    if(explicitGlobal) return false;
    return symTable.isInsideFunction() || (symTable.getScopeDepth() > 1);
}

bool Parser::isTopLevelProgramScope() const {
    return !symTable.isInsideFunction() && symTable.getScopeDepth() <= 1;
}

void Parser::rejectTopLevelExecutable(const std::string& construct) const {
    if(isTopLevelProgramScope()) {
        throw std::runtime_error(
            "Line " + std::to_string(currentToken.lineNumber) + ": " + construct
            + " is only allowed inside a function"
        );
    }
}

void Parser::validateTopLevelStatement(const std::shared_ptr<StatementNode>& stmt, int line) const {
    if(!stmt || !isTopLevelProgramScope()) return;

    if(std::dynamic_pointer_cast<FunctionDefNode>(stmt)) return;

    if(auto block = std::dynamic_pointer_cast<BlockCode>(stmt)) {
        for(const auto& child : block->getStatements()) {
            validateTopLevelStatement(child, line);
        }
        return;
    }

    if(auto assign = std::dynamic_pointer_cast<AssignmentNode>(stmt)) {
        if(!assign->isLocal()) return;
        throw std::runtime_error(
            "Line " + std::to_string(line) + ": local declarations are not allowed at top level"
        );
    }

    throw std::runtime_error(
        "Line " + std::to_string(line) + ": executable statements are only allowed inside a function"
    );
}

std::shared_ptr<ASTNode> Parser::createUnaryNode(const std::string& op,
    std::shared_ptr<ASTNode> child) {
    auto num = std::dynamic_pointer_cast<NumberNode>(child);
    if(num) {
        if(op == "-" || op == "_") return std::make_shared<NumberNode>(-num->getValue());
        if(op == "+" || op == "#") return num;
        if(op == "~") {
            long long val = static_cast<long long>(num -> getValue());
            return std::make_shared<NumberNode>(static_cast<double>(~val));
        }
    }
    if(op == "not") {
        if(num) return std::make_shared<NumberNode>(num -> getValue() == 0 ? 1.0 : 0.0);
        return std::make_shared<UnaryOpNode>("not", child);
    }
    return std::make_shared<UnaryOpNode>(op, child);
}

int Parser::precedence(const std::string& op) const {
    if(op == "or") return 0;
    if(op == "and") return 1;
    if(op == "==" || op == "!=" || op == ">" || op == "<" || op == ">=" || op == "<=") return 2;
    if(op == "|" || op == "^") return 3;
    if(op == "&") return 4;
    if(op == "<<" || op == ">>") return 5;
    if(op == "+" || op == "-") return 6;
    if(op == "*" || op == "/" || op == "%" || op == "//" || op == "%/") return 7;
    if(op == "not" || op == "_" || op == "#" || op == "~") return 8;
    if(op == "**") return 9;
    return -1;
}

void Parser::createNodeFromOp() {
    if(ops.empty()) return;
    std::string op = ops.top(); ops.pop();
    if(op == "_" || op == "#" || op == "not" || op == "~") {
        if(nodes.empty()) { state = ParserState::Error; return; }
        auto operand = nodes.top(); nodes.pop();
        nodes.push(createUnaryNode(op, operand));
    } else {
        if(nodes.size() < 2) { state = ParserState::Error; return; }
        auto right = nodes.top(); nodes.pop();
        auto left  = nodes.top(); nodes.pop();
        nodes.push(createBinaryNode(op, left, right));
    }
}

void Parser::processOperatorStack(const std::string& currentOp) {
    while(!ops.empty() && ops.top() != "(") {
        int topPrec = precedence(ops.top());
        int currentPrec = precedence(currentOp);
        if(topPrec < currentPrec) break;

        if(topPrec == currentPrec) {
            if(currentOp == "**") break;
        }
        createNodeFromOp();
    }
}

std::shared_ptr<StatementNode> Parser::parseProgram(bool requireMain) {
    programHasMain = false;
    symTable.beginProgramParse();
    auto block = std::make_shared<BlockCode>();
    while(currentToken.type != TokenType::EndOfExpr) {
        int stmtLine = currentToken.lineNumber;
        auto stmt = parseStatement();
        if(stmt) {
            validateTopLevelStatement(stmt, stmtLine);
            block->addStatement(stmt);
        } else if(currentToken.type == TokenType::Semicolon) {
            nextToken(); // allow optional ';' between top-level declarations (e.g. after '};')
        } else if(state == ParserState::Error) break;
        else nextToken();
    }
    symTable.endProgramParse();
    if(requireMain && !programHasMain) {
        error("Program must define function main() (void function main() { ... })");
    }
    return block;
}

std::shared_ptr<StatementNode> Parser::parseStatement() {
    int stmtLine = currentToken.lineNumber;
    if(currentToken.type == TokenType::Error) {
        error("Error Line " + std::to_string(currentToken.lineNumber) +
              " unknown operator: " + currentToken.value);
        return nullptr;
    }
    switch(currentToken.type) {
        case TokenType::If: {
            rejectTopLevelExecutable("if");
            auto s = parseIf();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::While: {
            rejectTopLevelExecutable("while");
            auto s = parseWhile();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }     
        case TokenType::OpenBrace: {
            rejectTopLevelExecutable("block");
            int blockLine = currentToken.lineNumber;
            auto s = parseBlock();
            if(s) s -> lineNumber = blockLine;
            return s;
        }
        case TokenType::Print: {
            rejectTopLevelExecutable("print");
            auto s = parsePrint();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::For: {
            rejectTopLevelExecutable("for");
            auto s = parseFor();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::Function:
        case TokenType::Void: {
            auto s = parseFunction();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::Return: {
            rejectTopLevelExecutable("return");
            auto s = parseReturn();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::Name: {
            auto s = parseAssignment();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::Switch: {
            rejectTopLevelExecutable("switch");
            auto s = parseSwitch();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }

        case TokenType::Local: case TokenType::Global: {
            bool isGlobal = (currentToken.type == TokenType::Global);
            nextToken(); // consume 'global' or 'local'
            if(currentToken.type == TokenType::Variable) {
                nextToken(); // skip 'variable'/'var'
                bool explicitGlobal = isGlobal;
                bool explicitLocal  = !isGlobal;
                if(explicitLocal && isTopLevelProgramScope()) {
                    error("'local variable' is not allowed in top-level scope");
                }
                auto s = parseVarDeclarationList(explicitLocal, explicitGlobal);
                if(s) s->lineNumber = stmtLine;
                return s;
            }
            
            {
                if(currentToken.type != TokenType::Name) {
                    state = ParserState::Error;
                    return nullptr;
                }
                
                bool explicitGlobal2 = isGlobal;
                bool explicitLocal2  = !isGlobal;
                if(explicitLocal2 && isTopLevelProgramScope()) {
                    error("'local' is not allowed in top-level scope");
                }
                std::string name2 = currentToken.value;
                nextToken();

                if(currentToken.type == TokenType::Semicolon) {
                    std::shared_ptr<ASTNode> zeroNode = std::make_shared<NoneNode>();
                    if(explicitLocal2) {
                        if(symTable.hasLocalInInnermostScope(name2))
                            error("Local variable redefinition is not allowed: " + name2);
                        int32_t off = symTable.getLocalOffset(name2);
                        nextToken();
                        auto s = std::make_shared<AssignmentNode>(off, zeroNode);
                        s->lineNumber = stmtLine;
                        return s;
                    } else {
                        if(symTable.hasGlobal(name2) && !symTable.isInsideFunction())
                            error("Global variable redefinition is not allowed: " + name2);
                        size_t addr = symTable.getGlobalAddress(name2);
                        nextToken();
                        auto s = std::make_shared<AssignmentNode>(addr, zeroNode);
                        s->lineNumber = stmtLine;
                        return s;
                    }
                }
                if(currentToken.type == TokenType::OpenParen) {
                    auto callNode = parseFunctionCall(name2);
                    if(currentToken.type == TokenType::Semicolon) nextToken();
                    auto s = std::make_shared<FunctionCallStatementNode>(callNode);
                    s->lineNumber = stmtLine;
                    return s;
                }
                if(currentToken.type != TokenType::Assign && currentToken.type != TokenType::CompoundAssign) {
                    state = ParserState::Error;
                    return nullptr;
                }
                std::string assignOp = currentToken.value;
                nextToken();
                auto valueExpr = parseExpression();
                if(currentToken.type == TokenType::Semicolon) nextToken();

                if(assignOp == "=") {
                    if(explicitLocal2 && symTable.hasLocalInInnermostScope(name2))
                        error("Local variable redefinition is not allowed: " + name2);
                    if(explicitGlobal2 && symTable.hasGlobal(name2) && !symTable.isInsideFunction())
                        error("Global variable redefinition is not allowed: " + name2);
                }

                std::shared_ptr<StatementNode> s2;
                if(assignOp != "=") {
                    int32_t localOffset = 0; size_t globalAddr = 0; int oh = 0;
                    std::shared_ptr<ASTNode> varNode;
                    if(explicitLocal2) {
                        if(!symTable.tryResolveLocal(name2, localOffset, oh) || oh != 0)
                            error("Undefined local variable in compound assignment: " + name2);
                        varNode = std::make_shared<VariableNode>(localOffset, oh);
                    } else if(explicitGlobal2) {
                        if(!symTable.tryGetGlobalAddress(name2, globalAddr))
                            error("Undefined global variable in compound assignment: " + name2);
                        varNode = std::make_shared<VariableNode>(globalAddr);
                    } else {
                        state = ParserState::Error; return nullptr;
                    }
                    std::string mathOp;
                    if(assignOp=="+=") mathOp="+"; else if(assignOp=="-=") mathOp="-";
                    else if(assignOp=="*=") mathOp="*"; else if(assignOp=="/=") mathOp="/";
                    else if(assignOp=="%=") mathOp="%"; else if(assignOp=="^=") mathOp="^";
                    valueExpr = std::make_shared<BinaryOpNode>(mathOp, varNode, valueExpr);
                    if(explicitLocal2) {
                        s2 = std::make_shared<AssignmentNode>(localOffset, valueExpr, oh);
                    } else {
                        size_t addr2 = symTable.getGlobalAddress(name2);
                        s2 = std::make_shared<AssignmentNode>(addr2, valueExpr);
                    }
                } else {
                    if(explicitLocal2) {
                        if(!symTable.hasLocalInInnermostScope(name2)) symTable.getLocalOffset(name2);
                        int32_t off2 = 0; int oh2 = 0;
                        symTable.tryResolveLocal(name2, off2, oh2);
                        s2 = std::make_shared<AssignmentNode>(off2, valueExpr, oh2);
                    } else {
                        size_t addr2 = symTable.getGlobalAddress(name2);
                        s2 = std::make_shared<AssignmentNode>(addr2, valueExpr);
                    }
                }
                if(s2) s2->lineNumber = stmtLine;
                return s2;
            }
        }
        case TokenType::Variable: {
            auto s = parseVarDecl();
            if(s) s -> lineNumber = stmtLine;
            return s;
        }
        case TokenType::Break: {
            rejectTopLevelExecutable("break");
            if(!insideLoop && !insideSwitch) {
                error("break statement outside of loop or switch");
            }
            auto s = std::make_shared<BreakNode>();
            nextToken(); // skip 'break'
            if(currentToken.type == TokenType::Semicolon) {
                nextToken(); // skip ';'
            }
            s -> lineNumber = stmtLine;
            return s;
        }

        case TokenType::Continue: {
            rejectTopLevelExecutable("continue");
            if(!insideLoop) {
                error("continue statement outside the loop");
            }
            auto s = std::make_shared<ContinueNode>();
            nextToken(); // skip 'continue'
            if(currentToken.type == TokenType::Semicolon) {
                nextToken(); // skip ';'
            }
            s -> lineNumber = stmtLine;
            return s;
        }

        default:
            if(isTopLevelProgramScope()) {
                if(currentToken.type == TokenType::Semicolon) {
                    nextToken();
                    return nullptr;
                }
                error("executable statements are only allowed inside a function");
            }
            parseExpression();
            if(currentToken.type == TokenType::Semicolon) nextToken();
            return nullptr;
    }
}

std::shared_ptr<StatementNode> Parser::parseSwitch() {
    nextToken(); // skip 'switch'
    if(currentToken.type != TokenType::OpenParen) {
        error("Expected '(' after switch");
    }
    nextToken(); // skip '('
    auto expr = parseExpression();
    if(!expr) { 
        state = ParserState::Error; 
        return nullptr;
    }
    if(currentToken.type != TokenType::CloseParen) { 
        error("Expected ')' after switch expression");
    }
    nextToken(); // skip ')'
    if(currentToken.type != TokenType::OpenBrace) {
        error("Expected '{' for switch body");
    }

    bool oldInsideSwitch = insideSwitch;
    insideSwitch = true;
    symTable.enterBlockScope();

    nextToken(); // skip '{'
    std::vector<CaseItem> cases;
    std::shared_ptr<StatementNode> defaultBody = nullptr;

    while(currentToken.type == TokenType::Case || currentToken.type == TokenType::Default) {
        if(currentToken.type == TokenType::Case) {
            nextToken(); // skip 'case'
            std::vector<std::shared_ptr<ASTNode>> values;
            do {
                auto val = parseExpression();
                if(!val) { state = ParserState::Error; return nullptr; }
                values.push_back(val);
                if(currentToken.type == TokenType::Comma)
                    nextToken();
                else
                    break;
            } while(true);

            if(currentToken.type != TokenType::Colon) {
                error("Expected ':' after case values");
            }
            nextToken(); // skip ':'

            
            auto caseBody = std::make_shared<BlockCode>();
            while(currentToken.type != TokenType::Case &&
                  currentToken.type != TokenType::Default &&
                  currentToken.type != TokenType::CloseBrace) {
                auto stmt = parseStatement();
                if(stmt) caseBody->addStatement(stmt);
                else if(state == ParserState::Error) break;
            }
            cases.push_back({std::move(values), caseBody});
        }
        else if(currentToken.type == TokenType::Default) {
            if(defaultBody) {
                error("Multiple default blocks in switch");
            }
            nextToken(); // skip 'default'
            if(currentToken.type != TokenType::Colon) {
                error("Expected ':' after default");
            }
            nextToken(); // skip ':'
            auto defBlock = std::make_shared<BlockCode>();
            while(currentToken.type != TokenType::Case &&
                  currentToken.type != TokenType::Default &&
                  currentToken.type != TokenType::CloseBrace) {
                auto stmt = parseStatement();
                if(stmt) defBlock->addStatement(stmt);
                else if(state == ParserState::Error) break;
            }
            defaultBody = defBlock;
        }
    }

    if(currentToken.type != TokenType::CloseBrace) {
        error("Expected '}' at end of switch");
    }
    nextToken(); // skip '}'

    symTable.exitBlockScope();
    insideSwitch = oldInsideSwitch;
    return std::make_shared<SwitchNode>(expr, std::move(cases), defaultBody);
}

std::shared_ptr<StatementNode> Parser::parseIf() {
    nextToken(); // skip 'if'
    if(currentToken.type != TokenType::OpenParen) { state = ParserState::Error; return nullptr; }
    nextToken();
    auto cond = parseExpression();
    if(currentToken.type != TokenType::CloseParen) { state = ParserState::Error; return nullptr; }
    nextToken();
    
    // Enter new scope for 'then' branch
    symTable.enterBlockScope();
    auto thenBr = parseStatement();
    symTable.exitBlockScope();
    
    std::shared_ptr<StatementNode> elseBr = nullptr;
    if(currentToken.type == TokenType::Else) {
        nextToken();
        // Enter new scope for 'else' branch
        symTable.enterBlockScope();
        elseBr = parseStatement();
        symTable.exitBlockScope();
    }
    return std::make_shared<IfStatementNode>(cond, thenBr, elseBr);
}

std::shared_ptr<StatementNode> Parser::parseWhile() {
    nextToken(); // skip 'while'
    if(currentToken.type != TokenType::OpenParen) { state = ParserState::Error; return nullptr; }
    nextToken();
    auto cond = parseExpression();
    if(currentToken.type != TokenType::CloseParen) { state = ParserState::Error; return nullptr; }
    nextToken();
    
    // Enter new scope for while body
    symTable.enterBlockScope();
    
    bool wasInloop = insideLoop;
    insideLoop = true;

    auto body = parseStatement();
    
    insideLoop = wasInloop;
    
    symTable.exitBlockScope();
    
    return std::make_shared<WhileStatementNode>(cond, body);
}

std::shared_ptr<StatementNode> Parser::parseBlock() {
    auto block = std::make_shared<BlockCode>();
    nextToken(); // skip '{'
    
    // Enter new scope for this block
    symTable.enterBlockScope();

    while(currentToken.type != TokenType::CloseBrace &&
          currentToken.type != TokenType::EndOfExpr) {
        auto stmt = parseStatement();
        if(stmt) block->addStatement(stmt);
        else if(state == ParserState::Error) {
            break;
        }
    }

    if(currentToken.type == TokenType::CloseBrace) {
        nextToken();
    }
    
    // Exit block scope
    symTable.exitBlockScope();
    
    return block;
}

std::shared_ptr<StatementNode> Parser::parseVarDecl() {
    nextToken(); // skip 'variable' / 'var'

  // var [local|global] a = 1, b = 2;
    bool explicitGlobal = false;
    bool explicitLocal  = false;

    if(currentToken.type == TokenType::Global) {
        explicitGlobal = true;
        nextToken();
    } else if(currentToken.type == TokenType::Local) {
        explicitLocal = true;
        nextToken();
        if(isTopLevelProgramScope()) {
            error("'local' is not allowed in top-level scope");
        }
    }

    return parseVarDeclarationList(explicitLocal, explicitGlobal);
}

std::shared_ptr<StatementNode> Parser::parseVarDeclarationList(bool explicitLocal, bool explicitGlobal) {
    auto block = std::make_shared<BlockCode>();
    block -> lineNumber = currentToken.lineNumber;

    std::vector<std::string> names;
    std::vector<std::shared_ptr<ASTNode>> inits;

    while(currentToken.type == TokenType::Name) {
        names.push_back(currentToken.value);
        nextToken();

        if(currentToken.type == TokenType::Assign) {
            // var a, b, c = 1, 2, 3  - '=' applies to the whole name list
            bool tupleAssign = names.size() > 1;
            for(const auto& init : inits) {
                if(init) tupleAssign = false;
            }
            if(tupleAssign) {
                inits.push_back(nullptr);
                break;
            }
            nextToken(); // skip '='
            inits.push_back(parseExpression());
            if(!inits.back()) return nullptr;
        } else {
            inits.push_back(nullptr);
        }

        if(currentToken.type == TokenType::Comma) {
            nextToken();
            continue;
        }
        break;
    }

    // Tuple unpack: var a, b, c = 3, 4, 5;
    if(currentToken.type == TokenType::Assign) {
        bool anyPerNameInit = false;
        for(const auto& init : inits) {
            if(init) anyPerNameInit = true;
        }
        if(anyPerNameInit) {
            error("Expected ';' after variable declaration list");
        }
        if(names.empty()) {
            error("Expected variable name");
        }

        nextToken(); // skip '='
        std::vector<std::shared_ptr<ASTNode>> rhsValues;
        do {
            rhsValues.push_back(parseExpression());
            if(!rhsValues.back()) return nullptr;
            if(currentToken.type == TokenType::Comma) {
                nextToken();
                continue;
            }
            break;
        } while(true);

        if(rhsValues.size() != names.size()) {
            error("Variable count (" + std::to_string(names.size()) +
                  ") does not match initializer count (" +
                  std::to_string(rhsValues.size()) + ")");
        }
        inits = std::move(rhsValues);
    }

    if(names.empty()) {
        error("Expected variable name");
    }

    const bool isLocalVar = explicitLocal ||
        (!explicitGlobal && shouldDefaultToLocal(explicitGlobal));

    for(size_t i = 0; i < names.size(); ++i) {
        const std::string& name = names[i];
        std::shared_ptr<ASTNode> valueExpr = inits[i];
        if(!valueExpr) {
            valueExpr = std::make_shared<NoneNode>();
        }

        if(explicitGlobal || (!explicitLocal && isTopLevelProgramScope())) {
            if(symTable.hasGlobal(name))
                error("Redeclaration of global variable: '" + name + "'");
        } else {
            if(symTable.hasLocalInInnermostScope(name))
                error("Redeclaration of variable '" + name + "' in the same scope");
        }

        std::shared_ptr<StatementNode> assign;
        if(isLocalVar) {
            int32_t off = symTable.getLocalOffset(name);
            assign = std::make_shared<AssignmentNode>(off, valueExpr);
        } else {
            size_t addr = symTable.getGlobalAddress(name);
            assign = std::make_shared<AssignmentNode>(addr, valueExpr);
        }
        assign -> lineNumber = block -> lineNumber;
        block -> addStatement(assign);
    }

    if(currentToken.type != TokenType::Semicolon) {
        error("Expected ';' after variable declaration list");
    }
    nextToken(); // consume ';'
    return block;
}

std::shared_ptr<StatementNode> Parser::parseAssignment(bool explicitDeclare) {
    (void)explicitDeclare; // reserved for future use; declaration is handled by parseVarDecl
    bool explicitLocal = false;
    bool explicitGlobal = false;
    std::string name;

    if (currentToken.type == TokenType::Local) {
        explicitLocal = true;
        nextToken();
        if (isTopLevelProgramScope()) {
            error("'local' is not allowed in top-level scope");
        }
    } 
    else if (currentToken.type == TokenType::Global) {
        explicitGlobal = true;
        nextToken();
    }

    if (currentToken.type != TokenType::Name) {
        state = ParserState::Error;
        return nullptr;
    }

    name = currentToken.value;
    nextToken();

    if(currentToken.type == TokenType::Semicolon) {
        bool isLocal;
        if (explicitLocal) isLocal = true;
        else if (explicitGlobal) isLocal = false;
        else isLocal = shouldDefaultToLocal(false);

        if (explicitLocal && symTable.hasLocalInInnermostScope(name)) {
            error("Local variable redefinition is not allowed: " + name);
        }
        // Only check global redefinition at top-level scope, not inside functions
        if (explicitGlobal && symTable.hasGlobal(name) && !symTable.isInsideFunction()) {
            error("Global variable redefinition is not allowed: " + name);
        }

        std::shared_ptr<ASTNode> zeroNode = std::make_shared<NoneNode>();
        if(isLocal) {
            int32_t off = symTable.getLocalOffset(name); // local, default "none"
            nextToken(); // skip ';'
            return std::make_shared<AssignmentNode>(off,zeroNode);
        } else {
            size_t addr = symTable.getGlobalAddress(name); // global, default "none"
            nextToken(); // skip ';'
            return std::make_shared<AssignmentNode>(addr, zeroNode);
        }
    }

    if (currentToken.type == TokenType::OpenParen) {
        auto callNode = parseFunctionCall(name);
        if (currentToken.type == TokenType::Semicolon) nextToken();
        return std::make_shared<FunctionCallStatementNode>(callNode);
    }

    std::shared_ptr<ASTNode> lhs;
    if (currentToken.type == TokenType::OpenBracket) {
        lhs = resolveVariableNode(name);
        if (!lhs) return nullptr;
        lhs = applySubscriptChain(lhs);
        if (!lhs) return nullptr;
    }

    bool isLocalVar = false;
    bool haveLocalBinding = false;
    int32_t resolvedLocalOff = 0;
    int resolvedOuterHops = 0;

    if (explicitLocal) {
        isLocalVar = true;
    } 
    else if (explicitGlobal) {
        isLocalVar = false;
    } 
    else {
        int32_t localOffset = 0;
        size_t globalAddr = 0;
        if(symTable.tryResolveLocal(name, localOffset, resolvedOuterHops)) {
            isLocalVar = true;
            haveLocalBinding = true;
            resolvedLocalOff = localOffset;
        } else if(symTable.tryGetGlobalAddress(name, globalAddr)) {
            isLocalVar = false;
        } else {
            isLocalVar = shouldDefaultToLocal(false);
        }
    }

    if(currentToken.type != TokenType::Assign && 
       currentToken.type != TokenType::CompoundAssign) {
        state = ParserState::Error;
        return nullptr;
    }

    std::string assignOp = currentToken.value;
    nextToken();

    auto valueExpr = parseExpression();

    if(currentToken.type == TokenType::Semicolon) {
        nextToken();
    }

    if (assignOp == "=") {
        if (explicitLocal && symTable.hasLocalInInnermostScope(name)) {
            error("Local variable redefinition is not allowed: " + name);
        }
        // Only check global redefinition at top-level scope, not inside functions
        if (explicitGlobal && symTable.hasGlobal(name) && !symTable.isInsideFunction()) {
            error("Global variable redefinition is not allowed: " + name);
        }
    }

    if (lhs) {
        auto subLhs = std::dynamic_pointer_cast<SubscriptReadNode>(lhs);
        if(!subLhs) {
            error("Invalid subscript assignment target");
            return nullptr;
        }
        if (assignOp != "=") {
            auto readNode = std::make_shared<SubscriptReadNode>(
                subLhs -> getObject(), subLhs -> getIndex()  
            );
            std::string mathOp;
            if (assignOp == "+=") mathOp = "+";
            else if (assignOp == "-=") mathOp = "-";
            else if (assignOp == "*=") mathOp = "*";
            else if (assignOp == "/=") mathOp = "/";
            else if (assignOp == "%=") mathOp = "%";
            else if (assignOp == "^=") mathOp = "^";
            else {
                error("Unsupported compound assignment operator for subscript");
                return nullptr;
            }
            auto newValueExpr = std::make_shared<BinaryOpNode>(mathOp, readNode, valueExpr);
            return std::make_shared<SubscriptWriteNode>(
                subLhs -> getObject(), subLhs -> getIndex(), newValueExpr
            );
        } else {
            return std::make_shared<SubscriptWriteNode>(
                subLhs->getObject(), subLhs->getIndex(), valueExpr
            );
        }
    }

    if(assignOp != "=") {
        int32_t localOffset = 0;
        size_t globalAddr = 0;

        if (explicitLocal) {
            int oh = 0;
            if (!symTable.tryResolveLocal(name, localOffset, oh) || oh != 0) {
                error("Undefined local variable in compound assignment: " + name);
            }
            isLocalVar = true;
            haveLocalBinding = true;
            resolvedLocalOff = localOffset;
            resolvedOuterHops = 0;
        } else if (explicitGlobal) {
            if (!symTable.tryGetGlobalAddress(name, globalAddr)) {
                error("Undefined global variable in compound assignment: " + name);
            }
            isLocalVar = false;
        } else if (symTable.tryResolveLocal(name, localOffset, resolvedOuterHops)) {
            isLocalVar = true;
            haveLocalBinding = true;
            resolvedLocalOff = localOffset;
        } else if (symTable.tryGetGlobalAddress(name, globalAddr)) {
            isLocalVar = false;
        } else {
            if(symTable.hasActiveScope()) {
                error("Undefined variable in compound assignment: " + name);
            } else {
                globalAddr = symTable.getGlobalAddress(name);
                isLocalVar = false;
            }
        }

        std::string mathOp;
        if(assignOp == "+=")      mathOp = "+";
        else if(assignOp == "-=") mathOp = "-";
        else if(assignOp == "*=") mathOp = "*";
        else if(assignOp == "/=") mathOp = "/";
        else if(assignOp == "%=") mathOp = "%";
        else if(assignOp == "^=") mathOp = "^";

        std::shared_ptr<ASTNode> varNode;
        if (isLocalVar) {
            varNode = std::make_shared<VariableNode>(localOffset, resolvedOuterHops);
        } else {
            varNode = std::make_shared<VariableNode>(globalAddr);
        }

        valueExpr = std::make_shared<BinaryOpNode>(mathOp, varNode, valueExpr);
    } else {
        if (isLocalVar) {
            if (!haveLocalBinding) {
                symTable.getLocalOffset(name);
            }
        } else {
            symTable.getGlobalAddress(name);
        }
    }

    if (isLocalVar) {
        int32_t off = haveLocalBinding ? resolvedLocalOff : symTable.getLocalOffset(name);
        int oh = haveLocalBinding ? resolvedOuterHops : 0;
        return std::make_shared<AssignmentNode>(off, valueExpr, oh);
    } else {
        size_t addr = symTable.getGlobalAddress(name);
        return std::make_shared<AssignmentNode>(addr, valueExpr);
    }
}

std::shared_ptr<StatementNode> Parser::parsePrint() {
    nextToken(); // skip 'print'
    if(currentToken.type != TokenType::OpenParen) { state = ParserState::Error; return nullptr; }
    nextToken(); // skip '('

    std::vector<std::shared_ptr<ASTNode>> exprs;
    while(currentToken.type != TokenType::CloseParen && currentToken.type != TokenType::EndOfExpr) {
        exprs.push_back(parseExpression());
        if(currentToken.type == TokenType::Comma) nextToken();
    }

    if(currentToken.type != TokenType::CloseParen) { state = ParserState::Error; return nullptr; }
    nextToken(); // skip ')'

    if(exprs.size() <= 1) {
        exprs.push_back(std::make_shared<StringNode>("\n"));
    }

    if(currentToken.type == TokenType::Semicolon) {
        nextToken();
    }

    return std::make_shared<PrintNode>(std::move(exprs));
}

std::shared_ptr<ASTNode> Parser::parseArrayLiteral() {
    nextToken(); // skip '['
    std::vector<std::shared_ptr<ASTNode>> elements;
    auto savedOps = ops;
    auto savedNodes = nodes;
    auto savedState = state;
    while(currentToken.type != TokenType::CloseBracket && currentToken.type != TokenType::EndOfExpr) {
        auto elem = parseExpression();
        if(state == ParserState::Error || !elem) {
            state = ParserState::Error;
            return nullptr;
        }
        elements.push_back(elem);
        ops = savedOps;
        nodes = savedNodes;
        state = savedState;
        if(currentToken.type == TokenType::Comma) {
            nextToken();
        }
    }
    if(currentToken.type != TokenType::CloseBracket) {
        error("Expected ']' to close array literal");
    }
    ops = savedOps;
    nodes = savedNodes;
    state = savedState;
    nextToken(); // skip ']'
    return std::make_shared<ArrayLiteralNode>(std::move(elements));
}

// Splits a raw f-string body (escapes already resolved by the tokenizer,
// but "{" / "}" left untouched) into alternating literal text and
// interpolated expressions: "x={x}" -> literals=["x=",""], expressions=[x].
// "{{" / "}}" are literal braces, matching the usual f-string convention.
// Each {expr} is parsed with its own throwaway Lexer/Tokenizer/Parser
// sharing this parser's SymbolTable, so variables in scope at the f-string's
// location resolve exactly as if the expression were written inline. Note:
// a nested string literal inside {expr} must use the OTHER quote character
// from the f-string's own quotes, since the tokenizer has already scanned
// past the f-string using a single matching-quote rule before parseFString
// ever sees the text - the same limitation Python has before 3.12.
std::shared_ptr<ASTNode> Parser::parseFString(const std::string& raw) {
    std::vector<std::string> literals;
    std::vector<std::shared_ptr<ASTNode>> expressions;
    std::string currentLiteral;

    size_t i = 0;
    while(i < raw.size()) {
        char c = raw[i];
        if(c == '{') {
            if(i + 1 < raw.size() && raw[i + 1] == '{') {
                currentLiteral += '{';
                i += 2;
                continue;
            }
            literals.push_back(currentLiteral);
            currentLiteral.clear();
            i++;
            std::string exprText;
            while(i < raw.size() && raw[i] != '}') {
                exprText += raw[i];
                i++;
            }
            if(i >= raw.size()) {
                error("f-string: missing closing '}' for interpolation");
                return nullptr;
            }
            i++; // skip '}'
            if(exprText.empty()) {
                error("f-string: empty expression in {}");
                return nullptr;
            }

            std::shared_ptr<ASTNode> exprNode;
            try {
                std::istringstream exprStream(exprText);
                Lexer exprLexer(exprStream);
                Tokenizer exprTokenizer(exprLexer);
                Parser exprParser(exprTokenizer, symTable);
                exprNode = exprParser.parseExpression();
            } catch(const std::exception& e) {
                // Strip the nested (throwaway) parser's own "Line N: " prefix
                // - its line counter always starts at 1 for the extracted
                // snippet and would be confusing next to the outer message's
                // real line number.
                std::string msg = e.what();
                size_t colonPos = msg.find(": ");
                if(msg.rfind("Line ", 0) == 0 && colonPos != std::string::npos) {
                    msg = msg.substr(colonPos + 2);
                }
                error("f-string: invalid expression '" + exprText + "' (" + msg + ")");
                return nullptr;
            }
            if(!exprNode) {
                error("f-string: invalid expression '" + exprText + "'");
                return nullptr;
            }
            expressions.push_back(exprNode);
        } else if(c == '}') {
            if(i + 1 < raw.size() && raw[i + 1] == '}') {
                currentLiteral += '}';
                i += 2;
                continue;
            }
            error("f-string: unmatched '}' - use '}}' for a literal '}'");
            return nullptr;
        } else {
            currentLiteral += c;
            i++;
        }
    }
    literals.push_back(currentLiteral);
    return std::make_shared<FStringNode>(std::move(literals), std::move(expressions));
}

std::shared_ptr<ASTNode> Parser::applySubscriptChain(std::shared_ptr<ASTNode> base) {
    while (currentToken.type == TokenType::OpenBracket) {
        nextToken(); // skip '['
        auto savedOps = ops;
        auto savedNodes = nodes;
        auto savedState = state;
        auto index = parseExpression();
        ops = savedOps;
        nodes = savedNodes;
        state = savedState;
        if (!index) {
            state = ParserState::Error;
            return nullptr;
        }
        if (currentToken.type != TokenType::CloseBracket) {
            error("Expected ']' after subscript index");
        }
        nextToken(); // skip ']'
        base = std::make_shared<SubscriptReadNode>(base, index);
    }
    return base;
}

std::shared_ptr<ASTNode> Parser::parseExpression() {
    state = ParserState::ExpectOperand;
    while(!ops.empty()) ops.pop();
    while(!nodes.empty()) nodes.pop();

    while(true) {
        Token token = currentToken;

        if(token.type == TokenType::Error) {
            error("Error Line " + std::to_string(token.lineNumber) +
                  " unknown operator: " + token.value);
            return nullptr;
        }
        
        if(token.type == TokenType::EndOfExpr ||
           token.type == TokenType::Semicolon ||
           token.type == TokenType::Comma ||
           token.type == TokenType::Colon ||
           (token.type == TokenType::CloseParen && ops.empty()) ||
           token.type == TokenType::CloseBracket) {
            break;
        }

        switch(state) {
            case ParserState::ExpectOperand:
                if(token.type == TokenType::Number) {
                    nodes.push(std::make_shared<NumberNode>(std::stod(token.value)));
                    state = ParserState::ExpectOperator;
                    nextToken();
                } else if(token.type == TokenType::Boolean) {
                    double val = (token.value == "true") ? 1.0 : 0.0;
                    nodes.push(std::make_shared<NumberNode>(val));
                    state = ParserState::ExpectOperator;
                    nextToken();
                
                    } else if(token.type == TokenType::Name) {
                        std::string name = token.value;
                        nextToken();
                        if(currentToken.type == TokenType::OpenParen) {
                            std::shared_ptr<ASTNode> node = parseFunctionCall(name);
                            if(!node) return nullptr;
                            nodes.push(applySubscriptChain(node));
                            state = ParserState::ExpectOperator;
                        } else {
                            auto varNode = resolveVariableNode(name);
                            if(!varNode) return nullptr;
                            nodes.push(applySubscriptChain(varNode));
                            state = ParserState::ExpectOperator;
                        }
                } else if(token.type == TokenType::StringLiteral || token.type == TokenType::FStringLiteral) {
                    std::shared_ptr<ASTNode> strNode = (token.type == TokenType::FStringLiteral)
                        ? parseFString(token.value)
                        : std::static_pointer_cast<ASTNode>(std::make_shared<StringNode>(token.value));
                    if(!strNode) return nullptr;
                    nodes.push(applySubscriptChain(strNode));
                    state = ParserState::ExpectOperator;
                    nextToken();
                }  else if(token.type == TokenType::Math_const_vars) {
                    OpCode constOp = (token.value == "m_pi") ? OpCode::CONST_PI : 
                                    (
                                        token.value == "m_e" ? OpCode::CONST_E : 
                                        (token.value == "m_inf" ? OpCode::CONST_INF : OpCode::CONST_MAX)
                                    );
                    nodes.push(std::make_shared<MathConstantNode>(constOp));
                    state = ParserState::ExpectOperator;
                    nextToken();
                } else if(token.type == TokenType::OpenParen) {
                    ops.push("("); 
                    nextToken();
                } else if(token.type == TokenType::Operator &&
                          (token.value == "-" || token.value == "+" || token.value == "~")) {
                    ops.push(token.value == "-" ? "_" : (token.value == "+" ? "#" : "~"));
                    nextToken();
                } else if(token.type == TokenType::Not) {
                    ops.push("not"); nextToken();
                } else if(token.type == TokenType::None) {
                    nodes.push(std::make_shared<NoneNode>());
                    state = ParserState::ExpectOperator;
                    nextToken();
                } else if(token.type == TokenType::OpenBracket) {
                    auto arr = parseArrayLiteral();
                    if(!arr) { state = ParserState::Error; return nullptr; }
                    nodes.push(applySubscriptChain(arr));
                    state = ParserState::ExpectOperator;
                } else {
                    state = ParserState::Error;
                }
                break;

            case ParserState::ExpectOperator:
                if(token.type == TokenType::QuestionMark) {
                    while(!ops.empty() && ops.top() != "(") {
                        createNodeFromOp();
                    }
                    if(state == ParserState::Error || nodes.empty()) {
                        state = ParserState::Error;
                        return nullptr;
                    }
                    nextToken(); // skip "?"
                    auto savedOps = std::move(ops);
                    auto savedNodes = std::move(nodes);
                    auto trueExpr = parseExpression();
                    ops = std::move(savedOps);
                    nodes = std::move(savedNodes);
                    if(currentToken.type != TokenType::Colon) {
                        state = ParserState::Error;
                        return nullptr;
                    }
                    nextToken(); // skip ":"
                    savedOps = std::move(ops);
                    savedNodes = std::move(nodes);
                    auto falseExpr = parseExpression();
                    ops = std::move(savedOps);
                    nodes = std::move(savedNodes);
                    if(!trueExpr || !falseExpr || nodes.empty()) {
                        state = ParserState::Error;
                        return nullptr;
                    }
                    auto cond = nodes.top(); nodes.pop();
                    nodes.push(std::make_shared<TernaryOpNode>(cond, trueExpr, falseExpr));
                    state = ParserState::ExpectOperator;
                    break;
                } else if(token.type == TokenType::Operator ||
                   token.type == TokenType::CompareOp || 
                   token.value == "and" || token.value == "or") {
                    processOperatorStack(token.value);
                    ops.push(token.value);
                    state = ParserState::ExpectOperand; 
                    nextToken();
                } else if(token.type == TokenType::StringLiteral || token.type == TokenType::FStringLiteral) {
                    processOperatorStack("+");
                    ops.push("+");
                    std::shared_ptr<ASTNode> strNode = (token.type == TokenType::FStringLiteral)
                        ? parseFString(token.value)
                        : std::static_pointer_cast<ASTNode>(std::make_shared<StringNode>(token.value));
                    if(!strNode) return nullptr;
                    nodes.push(strNode);
                    state = ParserState::ExpectOperator; 
                    nextToken();
                } else if(token.type == TokenType::CloseParen) {
                    while(!ops.empty() && ops.top() != "(") createNodeFromOp();
                    if(!ops.empty() && ops.top() == "(") {
                        ops.pop();
                        if (!nodes.empty()) {
                            nodes.top() = applySubscriptChain(nodes.top());
                        }
                        state = ParserState::ExpectOperator; nextToken();
                    } else if (ops.empty()) {
                        break;
                    } else {
                        state = ParserState::Error;
                    }
                } else if(token.type == TokenType::OpenBracket) {
                    if (nodes.empty()) { state = ParserState::Error; break; }
                    nodes.top() = applySubscriptChain(nodes.top());
                    state = ParserState::ExpectOperator;
                } else if(token.type == TokenType::Number ||
                          token.type == TokenType::Name   ||
                          token.type == TokenType::OpenParen ||
                          token.type == TokenType::StringLiteral) {
                    processOperatorStack("*"); ops.push("*");
                    if(token.type == TokenType::Number) {
                        nodes.push(std::make_shared<NumberNode>(std::stod(token.value)));
                        state = ParserState::ExpectOperator; nextToken();
                        } else if(token.type == TokenType::Name) {
                            std::string name = token.value;
                            nextToken();
                            if(currentToken.type == TokenType::OpenParen) {
                                std::shared_ptr<ASTNode> node = parseFunctionCall(name);
                                if(!node) return nullptr;
                                nodes.push(applySubscriptChain(node));
                            } else {
                                auto varNode = resolveVariableNode(name);
                                if(!varNode) return nullptr;
                                nodes.push(applySubscriptChain(varNode));
                            }
                            state = ParserState::ExpectOperator;
                        } else if(token.type == TokenType::StringLiteral) {
                            nodes.push(applySubscriptChain(std::make_shared<StringNode>(token.value)));
                            state = ParserState::ExpectOperator;
                            nextToken();
                    } else {
                        ops.push("(");
                        state = ParserState::ExpectOperand; nextToken();
                    }
                } else {
                    state = ParserState::Error;
                }
                break;
            default: break;
        }
        if(state == ParserState::Error) break;
    }
    if(state == ParserState::Error) return nullptr;
    while(state != ParserState::Error && !ops.empty()) {
        if(ops.top() == "(") {
            state = ParserState::Error;
            break;
        }
        createNodeFromOp();
    }
    if(state == ParserState::Error || nodes.size() != 1) {
        state = ParserState::Error;
        return nullptr;
    }
    return nodes.top();
}

std::shared_ptr<StatementNode> Parser::parseFor() {
    nextToken(); // skip 'for'
    if(currentToken.type != TokenType::OpenParen) { state = ParserState::Error; return nullptr; }
    nextToken(); // skip '('

    // Enter new scope for the entire for loop.
    symTable.enterBlockScope();

    // Init accepts: i = 0 | variable i = 0 | local variable i = 0 | global variable i = 0
    std::shared_ptr<StatementNode> init;
    if(currentToken.type == TokenType::Local || currentToken.type == TokenType::Global) {
        bool isGlobal = (currentToken.type == TokenType::Global);
        nextToken(); // skip 'local' / 'global'
        if(currentToken.type != TokenType::Variable) {
            error("Expected 'variable' or 'var' after '" +
                  std::string(isGlobal ? "global" : "local") +
                  "' in for-loop initializer");
        }
        nextToken(); // skip 'variable' / 'var'
        if(currentToken.type != TokenType::Name) {
            error("Expected variable name in for-loop initializer");
        }
        std::string varName = currentToken.value;
        nextToken(); // skip name
        if(isGlobal) {
            if(symTable.hasGlobal(varName))
                error("Redeclaration of global variable: '" + varName + "'");
        } else {
            if(symTable.hasLocalInInnermostScope(varName))
                error("Redeclaration of variable '" + varName + "' in the same scope");
        }
        std::shared_ptr<ASTNode> initExpr;
        if(currentToken.type == TokenType::Assign) {
            nextToken(); // skip '='
            initExpr = parseExpression();
        } else if(currentToken.type == TokenType::Semicolon) {
            initExpr = std::make_shared<NoneNode>();
        } else {
            error("Expected '=' or ';' after variable name in for-loop initializer");
        }
        if(currentToken.type == TokenType::Semicolon) nextToken();
        if(isGlobal) {
            size_t addr = symTable.getGlobalAddress(varName);
            init = std::make_shared<AssignmentNode>(addr, initExpr);
        } else {
            int32_t off = symTable.getLocalOffset(varName);
            init = std::make_shared<AssignmentNode>(off, initExpr);
        }
    } else if(currentToken.type == TokenType::Variable) {
        init = parseVarDecl();
    } else {
        init = parseAssignment();
    }

    // i <= 10;
    auto cond = parseExpression();
    if(currentToken.type == TokenType::Semicolon) nextToken();

    // i += 1;
    auto update = parseAssignment();

    if(currentToken.type != TokenType::CloseParen) {
        state = ParserState::Error;
        symTable.exitBlockScope();
        return nullptr;
    }
    nextToken();

    bool wasInLoop = insideLoop;
    insideLoop = true;

    // {...}
    auto body = parseStatement();

    insideLoop = wasInLoop;
    // Exit the for loop scope
    symTable.exitBlockScope();
    
    return std::make_shared<ForStatementNode>(init, cond, update, body);
}

std::shared_ptr<StatementNode> Parser::parseFunction() {

    bool isVoid = false;
    if (currentToken.type == TokenType::Void) {
        isVoid = true;
        nextToken();   // skip 'void'
    }

    if (currentToken.type != TokenType::Function) {
        error("Expected 'function' keyword");
    }
    nextToken(); // skip 'function'

    if (currentToken.type != TokenType::Name) {
        error("Expected function name");
    }
    std::string name = currentToken.value;
    nextToken();

    if (name == "main") {
        if(!isTopLevelProgramScope()) {
            error("function main() must be defined at top level");
        }
        if(programHasMain) {
            error("multiple definitions of function main()");
        }
        programHasMain = true;
    }

    if (currentToken.type != TokenType::OpenParen) {
        error("Expected '(' after function name");
    }
    nextToken(); // skip '('

    std::vector<ParamInfo> params;
    std::string variadicName;
    bool sawDefault = false;
    while (currentToken.type != TokenType::CloseParen && currentToken.type != TokenType::EndOfExpr) {
        bool isVariadicParam = false;
        if (currentToken.type == TokenType::Operator && currentToken.value == "*") {
            isVariadicParam = true;
            nextToken(); // skip '*'
        }

        if (currentToken.type != TokenType::Name) {
            error("Expected parameter name");
        }
        std::string paramName = currentToken.value;
        nextToken();

        if (isVariadicParam) {
            if (!variadicName.empty()) {
                error("Function '" + name + "' cannot have more than one *args parameter");
            }
            variadicName = paramName;
        } else {
            if (!variadicName.empty()) {
                error("Parameter '" + paramName + "' cannot follow the *args parameter");
            }
            std::shared_ptr<ASTNode> defaultExpr = nullptr;
            if (currentToken.type == TokenType::Assign) {
                nextToken(); // skip '='
                defaultExpr = parseExpression();
                sawDefault = true;
            } else if (sawDefault) {
                error("Parameter '" + paramName + "' without a default cannot follow a parameter with a default");
            }
            params.push_back({paramName, defaultExpr});
        }

        if (currentToken.type == TokenType::Comma) {
            nextToken();
        }
    }

    if (currentToken.type != TokenType::CloseParen) {
        error("Expected ')' after parameters");
    }
    nextToken(); // skip ')'

    if (name == "main" && (!params.empty() || !variadicName.empty())) {
        error("function main() must not take parameters");
    }

    if (currentToken.type != TokenType::OpenBrace) {
        error("Expected '{' before function body");
    }

    bool wasInsideFunction = insideFunction;
    insideFunction = true;
    symTable.enterFunctionScope();

    for (const auto& p : params) {
        symTable.getLocalOffset(p.name);
    }
    if (!variadicName.empty()) {
        symTable.getLocalOffset(variadicName);
    }

    auto body = parseBlock();

    if (currentToken.type == TokenType::Semicolon) {
        nextToken(); // optional ';' after function body (e.g. `};` at file scope)
    }

    if (!isVoid) {

        auto block = std::dynamic_pointer_cast<BlockCode>(body);
        if (!block) {
            symTable.exitFunctionScope();
            insideFunction = false;
            error("Function body is not a block");
        }

        const auto& statements = block->getStatements();
        bool hasReturn = false;
        if (!statements.empty()) {
            if (std::dynamic_pointer_cast<ReturnNode>(statements.back())) {
                hasReturn = true;
            }
        }
        if (!hasReturn) {
            symTable.exitFunctionScope();
            insideFunction = false;
            error("Non-void function '" + name + "' must end with a return statement");
        }
    }

    int slotCount = symTable.getLocalSlotCountForFrame();

    symTable.exitFunctionScope();
    insideFunction = wasInsideFunction;

    return std::make_shared<FunctionDefNode>(name, params, variadicName, body, slotCount, isVoid);
}

std::shared_ptr<ASTNode> Parser::parseFunctionCall(const std::string& name) {
    nextToken(); // skip '('
    std::vector<std::shared_ptr<ASTNode>> args;
    auto savedOps = ops;
    auto savedNodes = nodes;
    auto savedState = state;
    while(currentToken.type != TokenType::CloseParen && currentToken.type != TokenType::EndOfExpr) {
        auto arg = parseExpression();
        if (state == ParserState::Error || !arg) {
            return nullptr;
        }
        args.push_back(arg);
        ops = savedOps;
        nodes = savedNodes;
        state = savedState;
        if(currentToken.type == TokenType::Comma) nextToken();
    }
    if(currentToken.type != TokenType::CloseParen) {
        state = ParserState::Error;
        return nullptr;
    }
    ops = savedOps;
    nodes = savedNodes;
    state = savedState;
    nextToken(); // skip ')'
    return std::make_shared<FunctionCallNode>(name, std::move(args));
}

std::shared_ptr<StatementNode> Parser::parseReturn() {
    if (!insideFunction) {
        error("return is only allowed inside functions");
    }

    nextToken(); // skip 'return'

    std::shared_ptr<ASTNode> expr = nullptr;
    if (currentToken.type != TokenType::Semicolon && 
        currentToken.type != TokenType::EndOfExpr &&
        currentToken.type != TokenType::CloseBrace) {
        expr = parseExpression();
    }

    if (currentToken.type == TokenType::Semicolon) {
        nextToken();
    }

    return std::make_shared<ReturnNode>(expr);
}