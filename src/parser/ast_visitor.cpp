#include "ast_visitor.hpp"
#include "ast.hpp"

void ProgramNode::accept(ASTVisitor* visitor) {
    visitor->visitProgramNode(this);
}

void ProgramNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitProgramNode(this);
}

void FunctionDeclNode::accept(ASTVisitor* visitor) {
    visitor->visitFunctionDeclNode(this);
}

void FunctionDeclNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitFunctionDeclNode(this);
}

void StructDeclNode::accept(ASTVisitor* visitor) {
    visitor->visitStructDeclNode(this);
}

void StructDeclNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitStructDeclNode(this);
}

void ParamNode::accept(ASTVisitor* visitor) {
    visitor->visitParamNode(this);
}

void ParamNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitParamNode(this);
}

void BlockStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitBlockStmtNode(this);
}

void BlockStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitBlockStmtNode(this);
}

void VarDeclStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitVarDeclStmtNode(this);
}

void VarDeclStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitVarDeclStmtNode(this);
}

void IfStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitIfStmtNode(this);
}

void IfStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitIfStmtNode(this);
}

void WhileStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitWhileStmtNode(this);
}

void WhileStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitWhileStmtNode(this);
}

void ForStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitForStmtNode(this);
}

void ForStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitForStmtNode(this);
}

void ReturnStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitReturnStmtNode(this);
}

void ReturnStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitReturnStmtNode(this);
}

void ExprStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitExprStmtNode(this);
}

void ExprStmtNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitExprStmtNode(this);
}

void BinaryExprNode::accept(ASTVisitor* visitor) {
    visitor->visitBinaryExprNode(this);
}

void BinaryExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitBinaryExprNode(this);
}

void UnaryExprNode::accept(ASTVisitor* visitor) {
    visitor->visitUnaryExprNode(this);
}

void UnaryExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitUnaryExprNode(this);
}

void LiteralExprNode::accept(ASTVisitor* visitor) {
    visitor->visitLiteralExprNode(this);
}

void LiteralExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitLiteralExprNode(this);
}

void IdentifierExprNode::accept(ASTVisitor* visitor) {
    visitor->visitIdentifierExprNode(this);
}

void IdentifierExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitIdentifierExprNode(this);
}

void CallExprNode::accept(ASTVisitor* visitor) {
    visitor->visitCallExprNode(this);
}

void CallExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitCallExprNode(this);
}

void AssignmentExprNode::accept(ASTVisitor* visitor) {
    visitor->visitAssignmentExprNode(this);
}

void AssignmentExprNode::accept(ConstASTVisitor* visitor) const {
    visitor->visitAssignmentExprNode(this);
}
