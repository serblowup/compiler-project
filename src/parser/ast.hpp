#ifndef AST_HPP
#define AST_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include "../lexer/tokens.hpp"

namespace semantic {
    class Type;
    struct SymbolInfo;
}

class ASTVisitor;

class ASTNode {
protected:
    int line;
    int column;

public:
    ASTNode(int line, int column) : line(line), column(column) {}
    virtual ~ASTNode() = default;

    int getLine() const { return line; }
    int getColumn() const { return column; }

    virtual std::string toString() const = 0;
    virtual void accept(ASTVisitor* visitor) = 0;
};

class ExpressionNode : public ASTNode {
protected:
    semantic::Type* resolvedType;

public:
    ExpressionNode(int line, int column)
    : ASTNode(line, column), resolvedType(nullptr) {}
    virtual ~ExpressionNode() = default;

    semantic::Type* getType() const { return resolvedType; }

    void setType(semantic::Type* type) { resolvedType = type; }

    bool hasType() const { return resolvedType != nullptr; }
};

class StatementNode : public ASTNode {
public:
    using ASTNode::ASTNode;
    virtual ~StatementNode() = default;
};

class DeclarationNode : public ASTNode {
public:
    using ASTNode::ASTNode;
    virtual ~DeclarationNode() = default;
};

class ProgramNode : public ASTNode {
    std::vector<std::unique_ptr<DeclarationNode>> declarations;

public:
    ProgramNode(int line, int column) : ASTNode(line, column) {}

    void addDeclaration(std::unique_ptr<DeclarationNode> decl) {
        declarations.push_back(std::move(decl));
    }

    const std::vector<std::unique_ptr<DeclarationNode>>& getDeclarations() const {
        return declarations;
    }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class LiteralExprNode : public ExpressionNode {
    TokenType type;
    std::variant<int, double, std::string, bool> value;

public:
    LiteralExprNode(int line, int column, int val)
    : ExpressionNode(line, column), type(TokenType::tkn_INT_LITERAL), value(val) {}

    LiteralExprNode(int line, int column, double val)
    : ExpressionNode(line, column), type(TokenType::tkn_FLOAT_LITERAL), value(val) {}

    LiteralExprNode(int line, int column, const std::string& val)
    : ExpressionNode(line, column), type(TokenType::tkn_STRING_LITERAL), value(val) {}

    LiteralExprNode(int line, int column, bool val)
    : ExpressionNode(line, column), type(TokenType::tkn_BOOLEAN_LITERAL), value(val) {}

    TokenType getType() const { return type; }

    template<typename T>
    std::optional<T> getValue() const {
        if (std::holds_alternative<T>(value)) {
            return std::get<T>(value);
        }
        return std::nullopt;
    }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class IdentifierExprNode : public ExpressionNode {
    std::string name;
    semantic::SymbolInfo* symbol;

public:
    IdentifierExprNode(int line, int column, const std::string& name)
    : ExpressionNode(line, column), name(name), symbol(nullptr) {}

    const std::string& getName() const { return name; }

    semantic::SymbolInfo* getSymbol() const { return symbol; }

    void setSymbol(semantic::SymbolInfo* sym) { symbol = sym; }

    bool hasSymbol() const { return symbol != nullptr; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class BinaryExprNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode> left;
    TokenType op;
    std::unique_ptr<ExpressionNode> right;

public:
    BinaryExprNode(int line, int column,
                   std::unique_ptr<ExpressionNode> left,
                   TokenType op,
                   std::unique_ptr<ExpressionNode> right)
    : ExpressionNode(line, column), left(std::move(left)), op(op), right(std::move(right)) {}

    ExpressionNode* getLeft() { return left.get(); }
    const ExpressionNode* getLeft() const { return left.get(); }
    TokenType getOperator() const { return op; }
    ExpressionNode* getRight() { return right.get(); }
    const ExpressionNode* getRight() const { return right.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class UnaryExprNode : public ExpressionNode {
    TokenType op;
    std::unique_ptr<ExpressionNode> operand;

public:
    UnaryExprNode(int line, int column, TokenType op, std::unique_ptr<ExpressionNode> operand)
    : ExpressionNode(line, column), op(op), operand(std::move(operand)) {}

    TokenType getOperator() const { return op; }
    ExpressionNode* getOperand() { return operand.get(); }
    const ExpressionNode* getOperand() const { return operand.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class CallExprNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode> callee;
    std::vector<std::unique_ptr<ExpressionNode>> arguments;
    semantic::SymbolInfo* functionSymbol;

public:
    CallExprNode(int line, int column, std::unique_ptr<ExpressionNode> callee)
    : ExpressionNode(line, column), callee(std::move(callee)), functionSymbol(nullptr) {}

    void addArgument(std::unique_ptr<ExpressionNode> arg) {
        arguments.push_back(std::move(arg));
    }

    ExpressionNode* getCallee() { return callee.get(); }
    const ExpressionNode* getCallee() const { return callee.get(); }
    const std::vector<std::unique_ptr<ExpressionNode>>& getArguments() const {
        return arguments;
    }

    semantic::SymbolInfo* getFunctionSymbol() const { return functionSymbol; }

    void setFunctionSymbol(semantic::SymbolInfo* sym) { functionSymbol = sym; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class AssignmentExprNode : public ExpressionNode {
    std::unique_ptr<ExpressionNode> target;
    TokenType op;
    std::unique_ptr<ExpressionNode> value;

public:
    AssignmentExprNode(int line, int column,
                       std::unique_ptr<ExpressionNode> target,
                       TokenType op,
                       std::unique_ptr<ExpressionNode> value)
    : ExpressionNode(line, column), target(std::move(target)), op(op), value(std::move(value)) {}

    ExpressionNode* getTarget() { return target.get(); }
    const ExpressionNode* getTarget() const { return target.get(); }
    TokenType getOperator() const { return op; }
    ExpressionNode* getValue() { return value.get(); }
    const ExpressionNode* getValue() const { return value.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class BlockStmtNode : public StatementNode {
    std::vector<std::unique_ptr<StatementNode>> statements;

public:
    BlockStmtNode(int line, int column) : StatementNode(line, column) {}

    void addStatement(std::unique_ptr<StatementNode> stmt) {
        statements.push_back(std::move(stmt));
    }

    const std::vector<std::unique_ptr<StatementNode>>& getStatements() const {
        return statements;
    }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class ExprStmtNode : public StatementNode {
    std::unique_ptr<ExpressionNode> expression;

public:
    ExprStmtNode(int line, int column, std::unique_ptr<ExpressionNode> expr)
    : StatementNode(line, column), expression(std::move(expr)) {}

    ExpressionNode* getExpression() { return expression.get(); }
    const ExpressionNode* getExpression() const { return expression.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class IfStmtNode : public StatementNode {
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> thenBranch;
    std::unique_ptr<StatementNode> elseBranch;

public:
    IfStmtNode(int line, int column,
               std::unique_ptr<ExpressionNode> condition,
               std::unique_ptr<StatementNode> thenBranch,
               std::unique_ptr<StatementNode> elseBranch = nullptr)
    : StatementNode(line, column),
    condition(std::move(condition)),
    thenBranch(std::move(thenBranch)),
    elseBranch(std::move(elseBranch)) {}

    ExpressionNode* getCondition() { return condition.get(); }
    const ExpressionNode* getCondition() const { return condition.get(); }
    StatementNode* getThenBranch() { return thenBranch.get(); }
    const StatementNode* getThenBranch() const { return thenBranch.get(); }
    StatementNode* getElseBranch() { return elseBranch.get(); }
    const StatementNode* getElseBranch() const { return elseBranch.get(); }
    bool hasElse() const { return elseBranch != nullptr; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class WhileStmtNode : public StatementNode {
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<StatementNode> body;

public:
    WhileStmtNode(int line, int column,
                  std::unique_ptr<ExpressionNode> condition,
                  std::unique_ptr<StatementNode> body)
    : StatementNode(line, column), condition(std::move(condition)), body(std::move(body)) {}

    ExpressionNode* getCondition() { return condition.get(); }
    const ExpressionNode* getCondition() const { return condition.get(); }
    StatementNode* getBody() { return body.get(); }
    const StatementNode* getBody() const { return body.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class ForStmtNode : public StatementNode {
    std::unique_ptr<StatementNode> init;
    std::unique_ptr<ExpressionNode> condition;
    std::unique_ptr<ExpressionNode> update;
    std::unique_ptr<StatementNode> body;

public:
    ForStmtNode(int line, int column,
                std::unique_ptr<StatementNode> init,
                std::unique_ptr<ExpressionNode> condition,
                std::unique_ptr<ExpressionNode> update,
                std::unique_ptr<StatementNode> body)
    : StatementNode(line, column),
    init(std::move(init)),
    condition(std::move(condition)),
    update(std::move(update)),
    body(std::move(body)) {}

    StatementNode* getInit() { return init.get(); }
    const StatementNode* getInit() const { return init.get(); }
    ExpressionNode* getCondition() { return condition.get(); }
    const ExpressionNode* getCondition() const { return condition.get(); }
    ExpressionNode* getUpdate() { return update.get(); }
    const ExpressionNode* getUpdate() const { return update.get(); }
    StatementNode* getBody() { return body.get(); }
    const StatementNode* getBody() const { return body.get(); }

    bool hasInit() const { return init != nullptr; }
    bool hasCondition() const { return condition != nullptr; }
    bool hasUpdate() const { return update != nullptr; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class ReturnStmtNode : public StatementNode {
    std::unique_ptr<ExpressionNode> value;

public:
    ReturnStmtNode(int line, int column, std::unique_ptr<ExpressionNode> value = nullptr)
    : StatementNode(line, column), value(std::move(value)) {}

    ExpressionNode* getValue() { return value.get(); }
    const ExpressionNode* getValue() const { return value.get(); }
    bool hasValue() const { return value != nullptr; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class VarDeclStmtNode : public StatementNode {
    std::string type;
    std::string name;
    std::unique_ptr<ExpressionNode> initializer;

public:
    VarDeclStmtNode(int line, int column,
                    const std::string& type,
                    const std::string& name,
                    std::unique_ptr<ExpressionNode> initializer = nullptr)
    : StatementNode(line, column), type(type), name(name), initializer(std::move(initializer)) {}

    const std::string& getType() const { return type; }
    const std::string& getName() const { return name; }
    ExpressionNode* getInitializer() { return initializer.get(); }
    const ExpressionNode* getInitializer() const { return initializer.get(); }
    bool hasInitializer() const { return initializer != nullptr; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class ParamNode : public ASTNode {
    std::string type;
    std::string name;

public:
    ParamNode(int line, int column, const std::string& type, const std::string& name)
    : ASTNode(line, column), type(type), name(name) {}

    const std::string& getType() const { return type; }
    const std::string& getName() const { return name; }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class FunctionDeclNode : public DeclarationNode {
    std::string returnType;
    std::string name;
    std::vector<std::unique_ptr<ParamNode>> parameters;
    std::unique_ptr<BlockStmtNode> body;

public:
    FunctionDeclNode(int line, int column,
                     const std::string& returnType,
                     const std::string& name,
                     std::unique_ptr<BlockStmtNode> body)
    : DeclarationNode(line, column), returnType(returnType), name(name), body(std::move(body)) {}

    void addParameter(std::unique_ptr<ParamNode> param) {
        parameters.push_back(std::move(param));
    }

    const std::string& getReturnType() const { return returnType; }
    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<ParamNode>>& getParameters() const {
        return parameters;
    }
    BlockStmtNode* getBody() { return body.get(); }
    const BlockStmtNode* getBody() const { return body.get(); }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

class StructDeclNode : public DeclarationNode {
    std::string name;
    std::vector<std::unique_ptr<VarDeclStmtNode>> fields;

public:
    StructDeclNode(int line, int column, const std::string& name)
    : DeclarationNode(line, column), name(name) {}

    void addField(std::unique_ptr<VarDeclStmtNode> field) {
        fields.push_back(std::move(field));
    }

    const std::string& getName() const { return name; }
    const std::vector<std::unique_ptr<VarDeclStmtNode>>& getFields() const {
        return fields;
    }

    std::string toString() const override;
    void accept(ASTVisitor* visitor) override;
};

#endif
