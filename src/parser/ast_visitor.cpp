#include "ast_visitor.hpp"
#include "ast.hpp"

void ProgramNode::accept(ASTVisitor* visitor) {
    visitor->visitProgramNode(this);
}

void FunctionDeclNode::accept(ASTVisitor* visitor) {
    visitor->visitFunctionDeclNode(this);
}

void StructDeclNode::accept(ASTVisitor* visitor) {
    visitor->visitStructDeclNode(this);
}

void ParamNode::accept(ASTVisitor* visitor) {
    visitor->visitParamNode(this);
}

void BlockStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitBlockStmtNode(this);
}

void VarDeclStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitVarDeclStmtNode(this);
}

void IfStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitIfStmtNode(this);
}

void WhileStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitWhileStmtNode(this);
}

void ForStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitForStmtNode(this);
}

void ReturnStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitReturnStmtNode(this);
}

void ExprStmtNode::accept(ASTVisitor* visitor) {
    visitor->visitExprStmtNode(this);
}

void BinaryExprNode::accept(ASTVisitor* visitor) {
    visitor->visitBinaryExprNode(this);
}

void UnaryExprNode::accept(ASTVisitor* visitor) {
    visitor->visitUnaryExprNode(this);
}

void LiteralExprNode::accept(ASTVisitor* visitor) {
    visitor->visitLiteralExprNode(this);
}

void IdentifierExprNode::accept(ASTVisitor* visitor) {
    visitor->visitIdentifierExprNode(this);
}

void CallExprNode::accept(ASTVisitor* visitor) {
    visitor->visitCallExprNode(this);
}

void AssignmentExprNode::accept(ASTVisitor* visitor) {
    visitor->visitAssignmentExprNode(this);
}
