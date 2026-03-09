#ifndef AST_VISITOR_HPP
#define AST_VISITOR_HPP

class ProgramNode;
class FunctionDeclNode;
class StructDeclNode;
class ParamNode;
class BlockStmtNode;
class VarDeclStmtNode;
class IfStmtNode;
class WhileStmtNode;
class ForStmtNode;
class ReturnStmtNode;
class ExprStmtNode;
class BinaryExprNode;
class UnaryExprNode;
class LiteralExprNode;
class IdentifierExprNode;
class CallExprNode;
class AssignmentExprNode;

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;
    
    virtual void visitProgramNode(ProgramNode* node) = 0;
    virtual void visitFunctionDeclNode(FunctionDeclNode* node) = 0;
    virtual void visitStructDeclNode(StructDeclNode* node) = 0;
    virtual void visitParamNode(ParamNode* node) = 0;
    virtual void visitBlockStmtNode(BlockStmtNode* node) = 0;
    virtual void visitVarDeclStmtNode(VarDeclStmtNode* node) = 0;
    virtual void visitIfStmtNode(IfStmtNode* node) = 0;
    virtual void visitWhileStmtNode(WhileStmtNode* node) = 0;
    virtual void visitForStmtNode(ForStmtNode* node) = 0;
    virtual void visitReturnStmtNode(ReturnStmtNode* node) = 0;
    virtual void visitExprStmtNode(ExprStmtNode* node) = 0;
    virtual void visitBinaryExprNode(BinaryExprNode* node) = 0;
    virtual void visitUnaryExprNode(UnaryExprNode* node) = 0;
    virtual void visitLiteralExprNode(LiteralExprNode* node) = 0;
    virtual void visitIdentifierExprNode(IdentifierExprNode* node) = 0;
    virtual void visitCallExprNode(CallExprNode* node) = 0;
    virtual void visitAssignmentExprNode(AssignmentExprNode* node) = 0;
};

class ConstASTVisitor {
public:
    virtual ~ConstASTVisitor() = default;
    
    virtual void visitProgramNode(const ProgramNode* node) = 0;
    virtual void visitFunctionDeclNode(const FunctionDeclNode* node) = 0;
    virtual void visitStructDeclNode(const StructDeclNode* node) = 0;
    virtual void visitParamNode(const ParamNode* node) = 0;
    virtual void visitBlockStmtNode(const BlockStmtNode* node) = 0;
    virtual void visitVarDeclStmtNode(const VarDeclStmtNode* node) = 0;
    virtual void visitIfStmtNode(const IfStmtNode* node) = 0;
    virtual void visitWhileStmtNode(const WhileStmtNode* node) = 0;
    virtual void visitForStmtNode(const ForStmtNode* node) = 0;
    virtual void visitReturnStmtNode(const ReturnStmtNode* node) = 0;
    virtual void visitExprStmtNode(const ExprStmtNode* node) = 0;
    virtual void visitBinaryExprNode(const BinaryExprNode* node) = 0;
    virtual void visitUnaryExprNode(const UnaryExprNode* node) = 0;
    virtual void visitLiteralExprNode(const LiteralExprNode* node) = 0;
    virtual void visitIdentifierExprNode(const IdentifierExprNode* node) = 0;
    virtual void visitCallExprNode(const CallExprNode* node) = 0;
    virtual void visitAssignmentExprNode(const AssignmentExprNode* node) = 0;
};

#endif
