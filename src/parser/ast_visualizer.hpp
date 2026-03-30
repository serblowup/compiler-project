#ifndef AST_VISUALIZER_HPP
#define AST_VISUALIZER_HPP

#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include "ast.hpp"

// Визуализация AST
class ASTVisualizer {
public:
    virtual ~ASTVisualizer() = default;
    virtual std::string visualize(const ProgramNode* program) = 0;
    virtual void visualizeToFile(const ProgramNode* program, const std::string& filename) {
        std::ofstream file(filename);
        file << visualize(program);
    }
};

// Pretty printer
class PrettyPrinter : public ASTVisualizer {
public:
    std::string visualize(const ProgramNode* program) override;
};

// DOT формат
class DOTPrinter : public ASTVisualizer {
private:
    int nodeCounter;
    std::string generateNodeId();
    std::string escapeDOT(const std::string& str);
    std::string visitNode(const ASTNode* node, std::ostream& out);
    
public:
    DOTPrinter() : nodeCounter(0) {}
    std::string visualize(const ProgramNode* program) override;
};

// JSON формат
class JSONPrinter : public ASTVisualizer {
private:
    void visitNode(const ASTNode* node, std::ostringstream& out, int indent = 0);
    std::string escapeJSON(const std::string& str);
    
public:
    std::string visualize(const ProgramNode* program) override;
};

class CodeGenerator {
private:
    std::ostringstream out;
    int indentLevel;
    
    void indent();
    void dedent();
    void generateProgram(const ProgramNode* program);
    void generateDeclaration(const DeclarationNode* decl);
    void generateFunctionDecl(const FunctionDeclNode* func);
    void generateStructDecl(const StructDeclNode* structDecl);
    void generateVarDecl(const VarDeclStmtNode* varDecl);
    void generateStatement(const StatementNode* stmt);
    void generateBlock(const BlockStmtNode* block);
    void generateIfStmt(const IfStmtNode* ifStmt);
    void generateWhileStmt(const WhileStmtNode* whileStmt);
    void generateForStmt(const ForStmtNode* forStmt);
    void generateReturnStmt(const ReturnStmtNode* returnStmt);
    void generateExprStmt(const ExprStmtNode* exprStmt);
    void generateExpression(const ExpressionNode* expr);
    void generateBinaryExpr(const BinaryExprNode* bin);
    void generateUnaryExpr(const UnaryExprNode* un);
    void generateLiteralExpr(const LiteralExprNode* lit);
    void generateIdentifierExpr(const IdentifierExprNode* id);
    void generateCallExpr(const CallExprNode* call);
    void generateAssignmentExpr(const AssignmentExprNode* assign);
    void generateParam(const ParamNode* param);
    
public:
    CodeGenerator();
    std::string generate(const ProgramNode* program);
};

#endif