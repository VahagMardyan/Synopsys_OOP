#pragma once
#include <stack>
#include <memory>
#include <string>
#include "../Tokenizer/tokenizer.h"
#include "../AST/ast.h"
#include "../SymbolTable/symbol_table.h"

enum class ParserState { ExpectOperand, ExpectOperator, Done, Error };

class Parser {
private:
    bool insideFunction = false;
    bool insideLoop = false;
    bool insideSwitch = false;
    bool programHasMain = false;
    Tokenizer& tokenizer;
    SymbolTable& symTable;
    Token currentToken;
    std::stack<std::string> ops;
    std::stack<std::shared_ptr<ASTNode>> nodes;
    ParserState state;

    void nextToken() { currentToken = tokenizer.getNextToken(); }
    int precedence(const std::string& op) const;
    void processOperatorStack(const std::string& currentOp);
    void createNodeFromOp();
    std::shared_ptr<ASTNode> createBinaryNode(const std::string& op,
        std::shared_ptr<ASTNode> left, std::shared_ptr<ASTNode> right);
    std::shared_ptr<ASTNode> createUnaryNode(const std::string& op,
        std::shared_ptr<ASTNode> child);
    std::shared_ptr<ASTNode> resolveVariableNode(const std::string& name);
    bool shouldDefaultToLocal(bool explicitGlobal) const;

    bool isTopLevelProgramScope() const;
    void rejectTopLevelExecutable(const std::string& construct) const;
    void validateTopLevelStatement(const std::shared_ptr<StatementNode>& stmt, int line) const;

    std::shared_ptr<StatementNode> parseStatement();
    std::shared_ptr<StatementNode> parseIf();
    std::shared_ptr<StatementNode> parseWhile();
    std::shared_ptr<StatementNode> parseBlock();
    std::shared_ptr<StatementNode> parseAssignment(bool explicitDeclare = false);
    std::shared_ptr<StatementNode> parseVarDecl();
    std::shared_ptr<StatementNode> parseVarDeclarationList(bool explicitLocal, bool explicitGlobal);
    std::shared_ptr<StatementNode> parsePrint();
    std::shared_ptr<ASTNode> parseExpression();
    std::shared_ptr<StatementNode> parseFor();
    std::shared_ptr<StatementNode> parseFunction();
    std::shared_ptr<StatementNode> parseReturn();
    std::shared_ptr<ASTNode> parseFunctionCall(const std::string& name);
    std::shared_ptr<ASTNode> parseWalrus(const std::string& name);
    std::shared_ptr<ASTNode> parseArrayLiteral();
    std::shared_ptr<ASTNode> parseFString(const std::string& raw);
    std::shared_ptr<ASTNode> applySubscriptChain(std::shared_ptr<ASTNode> base);
    std::shared_ptr<StatementNode> parseSwitch();
public:
    Parser(Tokenizer& tok, SymbolTable& st)
        : tokenizer(tok), symTable(st), state(ParserState::ExpectOperand) { nextToken(); }

    std::shared_ptr<StatementNode> parseProgram(bool requireMain = true);
    void error(const std::string& message) {
        state = ParserState::Error;
        throw std::runtime_error(
            "Line " + std::to_string(currentToken.lineNumber) + ": " + message
        );
    }
};