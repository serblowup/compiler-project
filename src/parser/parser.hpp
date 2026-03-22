#ifndef PARSER_HPP
#define PARSER_HPP

#include <vector>
#include <memory>
#include <string>
#include "ast.hpp"
#include "error.hpp"
#include "../lexer/tokens.hpp"

class ParseError {
    std::string message;
    int line;
    int column;
    std::string suggestion;
    
public:
    ParseError(const std::string& msg, int line, int col, const std::string& sugg = "");
    const std::string& getMessage() const;
    int getLine() const;
    int getColumn() const;
    const std::string& getSuggestion() const;
    std::string toString() const;
};

class Parser {
    std::vector<Token> tokens;
    size_t current;
    bool hadError;
    std::vector<ParseError> errors;
    std::vector<FormattedError> formatted_errors;
    ErrorMetrics metrics;
    int recursion_depth;
    int max_recursion_depth;
    int max_iterations;
    bool in_error_recovery;
    std::vector<RecoverySuggestion> suggestions;
    
    std::string filename;
    std::string source_code;
    
    Token peek() const;
    Token previous() const;
    bool isAtEnd() const;
    bool check(TokenType type) const;
    bool check(const std::vector<TokenType>& types) const;
    Token advance();
    bool match(TokenType type);
    bool match(const std::vector<TokenType>& types);
    Token consume(TokenType type, const std::string& message);
    Token consume(const std::vector<TokenType>& types, const std::string& message);
    
    bool try_insert_token(TokenType expected, const Token& at_token);
    bool try_delete_token(const Token& at_token);
    bool try_replace_token(TokenType expected, const Token& at_token);
    bool try_phrase_level_recovery(TokenType expected, const Token& at_token);
    void synchronize();
    bool advanced_synchronize();
    bool is_sync_point();
    TokenType guess_expected_token(const std::string& context);
    std::vector<RecoverySuggestion> generate_suggestions(const Token& at_token);
    void error(const std::string& message);
    void error(const std::string& message, const Token& token);
    void reportError(const std::string& message, const Token& token, ErrorCode code);
    void reportError(const std::string& message, int line, int column, const std::string& token_lexeme, ErrorCode code);
    std::string get_suggestion(const std::string& message, const Token& token);
    std::string getContextLine(int line) const;
    std::string generatePointer(int column) const;
    
    std::unique_ptr<ProgramNode> parseProgram();
    std::unique_ptr<DeclarationNode> parseDeclaration();
    std::unique_ptr<FunctionDeclNode> parseFunctionDecl();
    std::unique_ptr<StructDeclNode> parseStructDecl();
    std::unique_ptr<VarDeclStmtNode> parseVarDecl();
    
    std::unique_ptr<StatementNode> parseStatement();
    std::unique_ptr<StatementNode> parseBlockStmt();
    std::unique_ptr<StatementNode> parseIfStmt();
    std::unique_ptr<StatementNode> parseWhileStmt();
    std::unique_ptr<StatementNode> parseForStmt();
    std::unique_ptr<StatementNode> parseReturnStmt();
    std::unique_ptr<StatementNode> parseExprStmt();
    std::unique_ptr<StatementNode> parseEmptyStmt();
    std::unique_ptr<StatementNode> parseElsePart();
    
    std::unique_ptr<BlockStmtNode> parseBlock();
    
    std::unique_ptr<ExpressionNode> parseExpression();
    std::unique_ptr<ExpressionNode> parseAssignment();
    std::unique_ptr<ExpressionNode> parseLogicalOr();
    std::unique_ptr<ExpressionNode> parseLogicalAnd();
    std::unique_ptr<ExpressionNode> parseEquality();
    std::unique_ptr<ExpressionNode> parseRelational();
    std::unique_ptr<ExpressionNode> parseAdditive();
    std::unique_ptr<ExpressionNode> parseMultiplicative();
    std::unique_ptr<ExpressionNode> parseUnary();
    std::unique_ptr<ExpressionNode> parsePostfix();
    std::unique_ptr<ExpressionNode> parsePrimary();
    
    std::string parseType();
    std::unique_ptr<ParamNode> parseParameter();
    std::vector<std::unique_ptr<ParamNode>> parseParameterList();
    std::vector<std::unique_ptr<ExpressionNode>> parseArgumentList();
    
public:
    Parser(const std::vector<Token>& tokens, const std::string& filename = "<stdin>", const std::string& source_code = "");
    std::unique_ptr<ProgramNode> parse();
    bool hasErrors() const;
    const std::vector<ParseError>& getErrors() const;
    const std::vector<FormattedError>& getFormattedErrors() const;
    const ErrorMetrics& getMetrics() const;
    const std::vector<RecoverySuggestion>& getSuggestions() const;
    void setMaxErrorCount(int max);
    void reset(const std::vector<Token>& newTokens);
};

#endif
