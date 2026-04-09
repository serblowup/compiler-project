#ifndef IR_GENERATOR_HPP
#define IR_GENERATOR_HPP

#include "ir.hpp"
#include "../parser/ast.hpp"
#include "../parser/ast_visitor.hpp"
#include "../semantic/symbol_table.hpp"
#include "../semantic/type.hpp"
#include <stack>
#include <unordered_map>

namespace ir {

class IRGenerator : public ASTVisitor {
private:
    // Состояние генерации
    IRProgram* program;
    IRFunction* current_function;
    BasicBlock* current_block;
    semantic::SymbolTable* symbol_table;
    
    // Счётчики
    int temp_counter;
    int label_counter;
    
    // Стек для результатов выражений
    std::stack<Operand> expr_stack;
    
    struct ShortCircuitContext {
        std::string true_label;
        std::string false_label;
        bool is_logical_and;
    };
    std::stack<ShortCircuitContext> short_circuit_stack;
    
    // Вспомогательные методы
    std::string newTemp();
    std::string newLabel();
    
    void emit(std::unique_ptr<Instruction> instr);
    void emit(InstrKind kind, const Operand& dest, const Operand& src1);
    void emit(InstrKind kind, const Operand& dest, const Operand& src1, const Operand& src2);
    
    Operand loadVariable(const std::string& name, semantic::Type* type);
    void storeVariable(const std::string& name, const Operand& value);
    
    semantic::Type* getTypeFromAST(ExpressionNode* node);
    
    // Генерация выражения (возвращает операнд с результатом)
    Operand generateExpression(ExpressionNode* expr);
    
    void generateLogicalExpression(BinaryExprNode* node, 
                                   const std::string& true_label,
                                   const std::string& false_label);
    
    // Visitor методы для объявлений
    void visitProgramNode(ProgramNode* node) override;
    void visitFunctionDeclNode(FunctionDeclNode* node) override;
    void visitStructDeclNode(StructDeclNode* node) override;
    void visitParamNode(ParamNode* node) override;
    
    // Visitor методы для операторов
    void visitBlockStmtNode(BlockStmtNode* node) override;
    void visitIfStmtNode(IfStmtNode* node) override;
    void visitWhileStmtNode(WhileStmtNode* node) override;
    void visitForStmtNode(ForStmtNode* node) override;
    void visitReturnStmtNode(ReturnStmtNode* node) override;
    void visitVarDeclStmtNode(VarDeclStmtNode* node) override;
    void visitExprStmtNode(ExprStmtNode* node) override;
    
    // Visitor методы для выражений
    void visitBinaryExprNode(BinaryExprNode* node) override;
    void visitUnaryExprNode(UnaryExprNode* node) override;
    void visitLiteralExprNode(LiteralExprNode* node) override;
    void visitIdentifierExprNode(IdentifierExprNode* node) override;
    void visitCallExprNode(CallExprNode* node) override;
    void visitAssignmentExprNode(AssignmentExprNode* node) override;
    
public:
    explicit IRGenerator(semantic::SymbolTable* sym_table);
    ~IRGenerator() override = default;
    
    IRProgram* generate(ProgramNode* program_ast);
    
    void reset();
};

}

#endif
