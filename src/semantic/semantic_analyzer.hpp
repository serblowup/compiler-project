#ifndef SEMANTIC_ANALYZER_HPP
#define SEMANTIC_ANALYZER_HPP

#include "symbol_table.hpp"
#include "type_checker.hpp"
#include "semantic_error.hpp"
#include "../parser/ast.hpp"
#include "../parser/ast_visitor.hpp" 
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <tuple>

namespace semantic {

    // Семантический анализатор
    class SemanticAnalyzer : public ASTVisitor {
    private:
        std::unique_ptr<SymbolTable> symbolTable;

        std::unique_ptr<ErrorCollector> errorCollector;

        std::string filename;

        std::string sourceCode;

        std::string currentFunction;

        Type* currentFunctionReturnType;

        bool inLoop;

        bool inFunctionBody;

        Type* errorType;
        
        struct MemoryLayoutInfo {
            std::string functionName;
            int totalStackSize = 0;
            std::vector<std::tuple<std::string, std::string, int, int>> variables; // name, type, offset, size
        };
        std::vector<MemoryLayoutInfo> memoryLayouts;
        
        struct TypeStats {
            int totalExpressions = 0;
            int typedExpressions = 0;
            int errorTypes = 0;
            std::map<std::string, int> typeDistribution;
        } typeStats;
        
        void recordType(Type* type);

        void reportError(int line, int column, SemanticErrorCode code, const std::string& message);
        void reportTypeMismatch(int line, int column, Type* expected, Type* actual, const std::string& context = "");
        void reportUndeclared(int line, int column, const std::string& name);
        void reportDuplicate(int line, int column, const std::string& name);

        std::string getContextLine(int line);
        std::string getPointer(int column);

        bool checkNotDeclared(const std::string& name, int line, int column);

        void annotateNode(ExpressionNode* node, Type* type);

        Type* resolveIdentifier(IdentifierExprNode* node);

        Type* checkBinaryOp(BinaryExprNode* node);

        Type* checkUnaryOp(UnaryExprNode* node);

        Type* checkAssignment(AssignmentExprNode* node);

        Type* checkFunctionCall(CallExprNode* node);
        
        void calculateMemoryLayout(FunctionDeclNode* func, int& currentOffset, std::vector<std::tuple<std::string, std::string, int, int>>& variables);

    public:
        SemanticAnalyzer(const std::string& filename = "<stdin>",
                         const std::string& sourceCode = "");

        void analyze(ProgramNode* program);

        ProgramNode* getDecoratedAST() { return nullptr; }

        SymbolTable* getSymbolTable() { return symbolTable.get(); }

        SymbolTable getSymbolTableSnapshot() const;

        const std::vector<SemanticError>& getErrors() const;

        bool hasErrors() const;

        std::string getAllErrors() const;

        void clear();
        
        std::string getDecoratedASTString() const;
        
        std::string getValidationReport() const;
        
        struct TypeStatistics {
            int totalExpressions = 0;
            int typedExpressions = 0;
            int errorTypes = 0;
            std::map<std::string, int> typeDistribution;
        };
        TypeStatistics getTypeStatistics() const;
        
        std::string getTypeHierarchy() const;
        
        std::vector<MemoryLayoutInfo> getMemoryLayout() const;

        // Visitor методы для обхода AST
        void visitProgramNode(ProgramNode* node) override;
        void visitFunctionDeclNode(FunctionDeclNode* node) override;
        void visitStructDeclNode(StructDeclNode* node) override;
        void visitParamNode(ParamNode* node) override;
        void visitBlockStmtNode(BlockStmtNode* node) override;
        void visitVarDeclStmtNode(VarDeclStmtNode* node) override;
        void visitIfStmtNode(IfStmtNode* node) override;
        void visitWhileStmtNode(WhileStmtNode* node) override;
        void visitForStmtNode(ForStmtNode* node) override;
        void visitReturnStmtNode(ReturnStmtNode* node) override;
        void visitExprStmtNode(ExprStmtNode* node) override;

        // Методы для выражений
        void visitBinaryExprNode(BinaryExprNode* node) override;
        void visitUnaryExprNode(UnaryExprNode* node) override;
        void visitLiteralExprNode(LiteralExprNode* node) override;
        void visitIdentifierExprNode(IdentifierExprNode* node) override;
        void visitCallExprNode(CallExprNode* node) override;
        void visitAssignmentExprNode(AssignmentExprNode* node) override;
    };

}

#endif
