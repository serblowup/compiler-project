#include "parser.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

Parser::Parser(const std::vector<Token>& tokens) 
    : tokens(tokens), current(0), hadError(false), 
      recursion_depth(0), max_recursion_depth(1000), 
      max_iterations(10000), in_error_recovery(false) {}

void Parser::reset(const std::vector<Token>& newTokens) {
    tokens = newTokens;
    current = 0;
    hadError = false;
    errors.clear();
    suggestions.clear();
    metrics = ErrorMetrics();
    recursion_depth = 0;
    in_error_recovery = false;
}

Token Parser::peek() const {
    if (isAtEnd()) {
        static Token eof_token;
        eof_token.type = TokenType::TOKEN_END_OF_FILE;
        eof_token.lexeme = "";
        eof_token.line = tokens.empty() ? 0 : tokens.back().line;
        eof_token.column = tokens.empty() ? 0 : tokens.back().column;
        eof_token.literal_type = LiteralType::LITERAL_NONE;
        return eof_token;
    }
    return tokens[current];
}

Token Parser::previous() const {
    if (current == 0) return tokens[0];
    return tokens[current - 1];
}

bool Parser::isAtEnd() const {
    if (current >= tokens.size()) return true;
    if (tokens[current].type == TokenType::TOKEN_END_OF_FILE) {
        return true;
    }
    return false;
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return false;
    return peek().type == type;
}

bool Parser::check(const std::vector<TokenType>& types) const {
    if (isAtEnd()) return false;
    TokenType currentType = peek().type;
    for (TokenType type : types) {
        if (currentType == type) return true;
    }
    return false;
}

Token Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

Token Parser::consume(TokenType type, const std::string& message) {
    if (check(type)) return advance();
    
    Token current_token = peek();
    
    if (!in_error_recovery && try_phrase_level_recovery(type, current_token)) {
        metrics.mark_recovered(RecoveryType::INSERT_TOKEN);
        Token inserted_token;
        inserted_token.type = type;
        inserted_token.line = current_token.line;
        inserted_token.column = current_token.column;
        inserted_token.lexeme = token_type_to_string(type);
        return inserted_token;
    }
    
    error(message, current_token);
    return current_token;
}

Token Parser::consume(const std::vector<TokenType>& types, const std::string& message) {
    if (check(types)) return advance();
    
    Token current_token = peek();
    
    for (TokenType type : types) {
        if (!in_error_recovery && try_phrase_level_recovery(type, current_token)) {
            metrics.mark_recovered(RecoveryType::INSERT_TOKEN);
            Token inserted_token;
            inserted_token.type = type;
            inserted_token.line = current_token.line;
            inserted_token.column = current_token.column;
            inserted_token.lexeme = token_type_to_string(type);
            return inserted_token;
        }
    }
    
    error(message, current_token);
    return current_token;
}

ParseError::ParseError(const std::string& msg, int line, int col, const std::string& sugg)
    : message(msg), line(line), column(col), suggestion(sugg) {}

const std::string& ParseError::getMessage() const { return message; }
int ParseError::getLine() const { return line; }
int ParseError::getColumn() const { return column; }
const std::string& ParseError::getSuggestion() const { return suggestion; }

std::string ParseError::toString() const {
    std::string result = "[" + std::to_string(line) + ":" + std::to_string(column) + 
           "] Syntax error: " + message;
    if (!suggestion.empty()) {
        result += "\n  Suggestion: " + suggestion;
    }
    return result;
}

TokenType Parser::guess_expected_token(const std::string& context) {
    if (context.find("';'") != std::string::npos) return TokenType::tkn_SEMICOLON;
    if (context.find("'}'") != std::string::npos) return TokenType::tkn_RBRACE;
    if (context.find("')'") != std::string::npos) return TokenType::tkn_RPAREN;
    if (context.find("'('") != std::string::npos) return TokenType::tkn_LPAREN;
    if (context.find("'{'") != std::string::npos) return TokenType::tkn_LBRACE;
    if (context.find("identifier") != std::string::npos) return TokenType::tkn_IDENTIFIER;
    if (context.find("type") != std::string::npos) return TokenType::tkn_INT;
    return TokenType::TOKEN_ERROR;
}

bool Parser::try_insert_token(TokenType expected, const Token& at_token) {
    (void)at_token;
    if (in_error_recovery) return false;
    
    switch (expected) {
        case TokenType::tkn_SEMICOLON:
            if (check(TokenType::tkn_RBRACE) || check(TokenType::tkn_RPAREN) || isAtEnd()) {
                std::cerr << "  Восстановление: вставлена пропущенная ';'" << std::endl;
                return true;
            }
            break;
        case TokenType::tkn_RBRACE:
            if (isAtEnd() || check(TokenType::tkn_FN) || check(TokenType::tkn_STRUCT)) {
                std::cerr << "  Восстановление: вставлена пропущенная '}'" << std::endl;
                return true;
            }
            break;
        case TokenType::tkn_RPAREN:
            if (check(TokenType::tkn_SEMICOLON) || check(TokenType::tkn_LBRACE)) {
                std::cerr << "  Восстановление: вставлена пропущенная ')'" << std::endl;
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool Parser::try_delete_token(const Token& at_token) {
    if (in_error_recovery) return false;
    
    switch (at_token.type) {
        case TokenType::tkn_SEMICOLON:
            if (check(TokenType::tkn_SEMICOLON)) {
                std::cerr << "  Восстановление: удалена лишняя ';'" << std::endl;
                advance();
                return true;
            }
            break;
        case TokenType::tkn_RBRACE:
            if (previous().type == TokenType::tkn_LBRACE) {
                std::cerr << "  Восстановление: удалена лишняя '}'" << std::endl;
                advance();
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool Parser::try_replace_token(TokenType expected, const Token& at_token) {
    if (in_error_recovery) return false;
    
    if (expected == TokenType::tkn_SEMICOLON && at_token.type == TokenType::tkn_COMMA) {
        std::cerr << "  Восстановление: ',' заменена на ';'" << std::endl;
        return true;
    }
    if (expected == TokenType::tkn_RBRACE && at_token.type == TokenType::tkn_RPAREN) {
        std::cerr << "  Восстановление: ')' заменена на '}'" << std::endl;
        return true;
    }
    return false;
}

bool Parser::try_phrase_level_recovery(TokenType expected, const Token& at_token) {
    if (try_insert_token(expected, at_token)) return true;
    if (try_delete_token(at_token)) return true;
    if (try_replace_token(expected, at_token)) return true;
    return false;
}

std::vector<RecoverySuggestion> Parser::generate_suggestions(const Token& at_token) {
    std::vector<RecoverySuggestion> suggs;
    
    if (at_token.type == TokenType::tkn_SEMICOLON) {
        suggs.emplace_back("Лишняя точка с запятой", "Удалите лишнюю ';'", 
                          TokenType::TOKEN_ERROR, at_token.type, at_token.line, at_token.column);
    }
    
    if (check(TokenType::tkn_RBRACE) && previous().type != TokenType::tkn_LBRACE) {
        suggs.emplace_back("Пропущена точка с запятой", "Вставьте ';' перед '}'", 
                          TokenType::tkn_SEMICOLON, at_token.type, at_token.line, at_token.column);
    }
    
    if (at_token.type == TokenType::tkn_IDENTIFIER && at_token.lexeme.find_first_of("0123456789") == 0) {
        suggs.emplace_back("Некорректное имя", "Идентификатор не может начинаться с цифры", 
                          TokenType::tkn_IDENTIFIER, at_token.type, at_token.line, at_token.column);
    }
    
    return suggs;
}

std::string Parser::get_suggestion(const std::string& message, const Token& token) {
    std::string suggestion;
    
    if (message.find("';'") != std::string::npos) {
        suggestion = "Возможно, пропущена точка с запятой ';'";
        if (check(TokenType::tkn_RBRACE)) {
            suggestion += " перед '}'";
        }
    } else if (message.find("'}'") != std::string::npos) {
        suggestion = "Возможно, не хватает закрывающей фигурной скобки '}'";
    } else if (message.find("')'") != std::string::npos) {
        suggestion = "Возможно, не хватает закрывающей круглой скобки ')'";
    } else if (message.find("'('") != std::string::npos) {
        suggestion = "Возможно, не хватает открывающей круглой скобки '('";
    } else if (message.find("имя переменной") != std::string::npos) {
        if (!token.lexeme.empty() && token.lexeme.find_first_of("0123456789") == 0) {
            suggestion = "Имя переменной не может начинаться с цифры";
        } else {
            suggestion = "Имя переменной должно быть идентификатором";
        }
    } else if (message.find("имя функции") != std::string::npos) {
        suggestion = "После 'fn' должно следовать имя функции";
    } else if (message.find("выражение") != std::string::npos) {
        suggestion = "Проверьте синтаксис выражения";
    } else if (message.find("тип") != std::string::npos) {
        suggestion = "Используйте один из типов: int, float, bool, void, struct";
    } else if (message.find("объявление") != std::string::npos) {
        suggestion = "Ожидалось объявление функции, структуры или переменной";
    } else if (message.find("тело функции") != std::string::npos) {
        suggestion = "После объявления функции должно следовать тело в фигурных скобках {}";
    }
    
    auto extra_suggs = generate_suggestions(token);
    if (!extra_suggs.empty() && suggestion.empty()) {
        suggestion = extra_suggs[0].suggestion;
    }
    
    return suggestion;
}

void Parser::error(const std::string& message) {
    error(message, peek());
}

void Parser::error(const std::string& message, const Token& token) {
    if (metrics.limit_reached) return;
    
    hadError = true;
    std::string suggestion = get_suggestion(message, token);
    errors.emplace_back(message, token.line, token.column, suggestion);
    metrics.mark_error();
    
    std::cerr << "[" << token.line << ":" << token.column << "] Ошибка синтаксиса: " 
              << message << ", получен '" << token.lexeme << "'" << std::endl;
    
    if (!suggestion.empty()) {
        std::cerr << "  Подсказка: " << suggestion << std::endl;
    }
    
    auto suggs = generate_suggestions(token);
    suggestions.insert(suggestions.end(), suggs.begin(), suggs.end());
}

bool Parser::is_sync_point() {
    if (isAtEnd()) return true;
    switch (peek().type) {
        case TokenType::tkn_SEMICOLON:
        case TokenType::tkn_FN:
        case TokenType::tkn_STRUCT:
        case TokenType::tkn_IF:
        case TokenType::tkn_WHILE:
        case TokenType::tkn_FOR:
        case TokenType::tkn_RETURN:
        case TokenType::tkn_LBRACE:
        case TokenType::tkn_RBRACE:
        case TokenType::tkn_RPAREN:
        case TokenType::tkn_COMMA:
            return true;
        default:
            return false;
    }
}

bool Parser::advanced_synchronize() {
    in_error_recovery = true;
    int tokens_skipped = 0;
    int max_skip = 20;
    bool synchronized = false;
    
    while (!isAtEnd() && tokens_skipped < max_skip) {
        if (is_sync_point()) {
            synchronized = true;
            break;
        }
        advance();
        tokens_skipped++;
    }
    
    if (synchronized) {
        metrics.mark_recovered(RecoveryType::PANIC_MODE);
    }
    
    in_error_recovery = false;
    return synchronized;
}

void Parser::synchronize() {
    advance();
    
    while (!isAtEnd()) {
        if (previous().type == TokenType::tkn_SEMICOLON) return;
        
        switch (peek().type) {
            case TokenType::tkn_FN:
            case TokenType::tkn_STRUCT:
            case TokenType::tkn_INT:
            case TokenType::tkn_FLOAT:
            case TokenType::tkn_BOOL:
            case TokenType::tkn_VOID:
            case TokenType::tkn_IF:
            case TokenType::tkn_WHILE:
            case TokenType::tkn_FOR:
            case TokenType::tkn_RETURN:
            case TokenType::tkn_LBRACE:
            case TokenType::tkn_RBRACE:
                return;
            default:
                advance();
        }
    }
}

std::unique_ptr<ProgramNode> Parser::parse() {
    return parseProgram();
}

std::unique_ptr<ProgramNode> Parser::parseProgram() {
    auto program = std::make_unique<ProgramNode>(1, 1);
    int iterations = 0;
    
    while (!isAtEnd() && iterations < max_iterations && !metrics.limit_reached) {
        iterations++;
        
        if (peek().type == TokenType::TOKEN_END_OF_FILE) break;
        
        while (!isAtEnd() && peek().type == TokenType::tkn_SEMICOLON) {
            advance();
        }
        
        if (isAtEnd()) break;
        
        if (peek().type == TokenType::tkn_RBRACE) {
            error("Неожиданная закрывающая скобка '}' на верхнем уровне");
            advance();
            continue;
        }
        
        try {
            auto decl = parseDeclaration();
            if (decl) {
                program->addDeclaration(std::move(decl));
            } else {
                if (!advanced_synchronize()) break;
            }
        } catch (const std::exception& e) {
            error(std::string("Фатальная ошибка: ") + e.what());
            if (!advanced_synchronize()) break;
        }
    }
    
    return program;
}

std::unique_ptr<DeclarationNode> Parser::parseDeclaration() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе объявления");
        recursion_depth--;
        return nullptr;
    }
    
    std::unique_ptr<DeclarationNode> result = nullptr;
    
    if (match(TokenType::tkn_FN)) {
        result = parseFunctionDecl();
    } else if (match(TokenType::tkn_STRUCT)) {
        result = parseStructDecl();
    } else if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
                      TokenType::tkn_BOOL, TokenType::tkn_VOID,
                      TokenType::tkn_STRUCT, TokenType::tkn_IDENTIFIER})) {
        auto varDecl = parseVarDecl();
        if (varDecl) {
            result = std::unique_ptr<DeclarationNode>(
                dynamic_cast<DeclarationNode*>(varDecl.release()));
        }
    } else {
        error("Ожидалось объявление (fn, struct или объявление переменной)");
    }
    
    recursion_depth--;
    return result;
}

std::unique_ptr<FunctionDeclNode> Parser::parseFunctionDecl() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе функции");
        recursion_depth--;
        return nullptr;
    }
    
    Token nameToken = consume(TokenType::tkn_IDENTIFIER, "Ожидалось имя функции");
    
    consume(TokenType::tkn_LPAREN, "Ожидалась '(' после имени функции");
    auto params = parseParameterList();
    consume(TokenType::tkn_RPAREN, "Ожидалась ')' после списка параметров");
    
    std::string returnType = "void";
    if (match(TokenType::tkn_ARROW)) {
        returnType = parseType();
    }
    
    if (!check(TokenType::tkn_LBRACE)) {
        error("Ожидалось тело функции (блок { ... })");
        int recover_iterations = 0;
        while (!isAtEnd() && !check(TokenType::tkn_LBRACE) && 
               !check(TokenType::tkn_SEMICOLON) && recover_iterations < 100) {
            advance();
            recover_iterations++;
        }
        metrics.mark_recovered(RecoveryType::PANIC_MODE);
    }
    
    auto body = parseBlock();
    if (!body) {
        error("Ожидалось тело функции");
        recursion_depth--;
        return nullptr;
    }
    
    auto func = std::make_unique<FunctionDeclNode>(
        nameToken.line, nameToken.column,
        returnType, nameToken.lexeme,
        std::move(body)
    );
    
    for (auto& param : params) {
        func->addParameter(std::move(param));
    }
    
    recursion_depth--;
    return func;
}

std::unique_ptr<StructDeclNode> Parser::parseStructDecl() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе структуры");
        recursion_depth--;
        return nullptr;
    }
    
    Token structToken = previous();
    Token nameToken = consume(TokenType::tkn_IDENTIFIER, "Ожидалось имя структуры");
    
    consume(TokenType::tkn_LBRACE, "Ожидалась '{' после имени структуры");
    
    auto structDecl = std::make_unique<StructDeclNode>(
        nameToken.line, nameToken.column,
        nameToken.lexeme
    );
    
    int iterations = 0;
    while (!check(TokenType::tkn_RBRACE) && !isAtEnd() && iterations < max_iterations) {
        iterations++;
        
        while (check(TokenType::tkn_SEMICOLON)) {
            advance();
        }
        
        if (check(TokenType::tkn_RBRACE)) break;
        
        if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
                   TokenType::tkn_BOOL, TokenType::tkn_VOID,
                   TokenType::tkn_STRUCT, TokenType::tkn_IDENTIFIER})) {
            auto field = parseVarDecl();
            if (field) {
                structDecl->addField(std::move(field));
            }
        } else {
            error("Ожидалось объявление поля");
            if (!advanced_synchronize()) break;
        }
    }
    
    if (isAtEnd() && !check(TokenType::tkn_RBRACE)) {
        error("Неожиданный конец файла - отсутствует закрывающая скобка '}' для структуры", nameToken);
        recursion_depth--;
        return structDecl;
    }
    
    consume(TokenType::tkn_RBRACE, "Ожидалась '}' после полей структуры");
    
    if (check(TokenType::tkn_SEMICOLON)) {
        advance();
    }
    
    recursion_depth--;
    return structDecl;
}

std::unique_ptr<VarDeclStmtNode> Parser::parseVarDecl() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе объявления переменной");
        recursion_depth--;
        return nullptr;
    }
    
    std::string type = parseType();
    Token nameToken = consume(TokenType::tkn_IDENTIFIER, "Ожидалось имя переменной");
    
    std::unique_ptr<ExpressionNode> initializer = nullptr;
    if (match(TokenType::tkn_ASSIGNMENT)) {
        initializer = parseExpression();
    }
    
    if (check(TokenType::tkn_SEMICOLON)) {
        advance();
    } else if (!check(TokenType::tkn_RBRACE) && !isAtEnd()) {
        error("Ожидалась ';' после объявления переменной");
        
        if (!in_error_recovery) {
            in_error_recovery = true;
            int recover_iterations = 0;
            while (!isAtEnd() && !check(TokenType::tkn_SEMICOLON) && 
                   !check(TokenType::tkn_RBRACE) && !check(TokenType::tkn_FN) &&
                   !check(TokenType::tkn_STRUCT) && recover_iterations < 100) {
                advance();
                recover_iterations++;
            }
            if (check(TokenType::tkn_SEMICOLON)) {
                advance();
                metrics.mark_recovered(RecoveryType::PANIC_MODE);
            }
            in_error_recovery = false;
        }
    }
    
    recursion_depth--;
    return std::make_unique<VarDeclStmtNode>(
        nameToken.line, nameToken.column,
        type, nameToken.lexeme,
        std::move(initializer)
    );
}

std::string Parser::parseType() {
    if (match({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
               TokenType::tkn_BOOL, TokenType::tkn_VOID})) {
        return previous().lexeme;
    } else if (match(TokenType::tkn_STRUCT)) {
        if (match(TokenType::tkn_IDENTIFIER)) {
            return "struct " + previous().lexeme;
        } else {
            error("Ожидалось имя структуры после 'struct'");
            return "struct";
        }
    } else if (match(TokenType::tkn_IDENTIFIER)) {
        return previous().lexeme;
    }
    
    error("Ожидался тип (int, float, bool, void, struct или идентификатор)");
    return "int";
}

std::unique_ptr<ParamNode> Parser::parseParameter() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе параметра");
        recursion_depth--;
        return nullptr;
    }
    
    std::string type = parseType();
    Token nameToken = consume(TokenType::tkn_IDENTIFIER, "Ожидалось имя параметра");
    
    recursion_depth--;
    return std::make_unique<ParamNode>(
        nameToken.line, nameToken.column,
        type, nameToken.lexeme
    );
}

std::vector<std::unique_ptr<ParamNode>> Parser::parseParameterList() {
    std::vector<std::unique_ptr<ParamNode>> params;
    
    if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
               TokenType::tkn_BOOL, TokenType::tkn_VOID,
               TokenType::tkn_STRUCT, TokenType::tkn_IDENTIFIER})) {
        auto param = parseParameter();
        if (param) {
            params.push_back(std::move(param));
        }
        
        int iterations = 0;
        while (match(TokenType::tkn_COMMA) && iterations < max_iterations) {
            iterations++;
            if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
                       TokenType::tkn_BOOL, TokenType::tkn_VOID,
                       TokenType::tkn_STRUCT, TokenType::tkn_IDENTIFIER})) {
                param = parseParameter();
                if (param) {
                    params.push_back(std::move(param));
                }
            } else {
                error("Ожидался тип параметра после ','");
                break;
            }
        }
    }
    
    return params;
}

std::unique_ptr<StatementNode> Parser::parseStatement() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе инструкции");
        recursion_depth--;
        return nullptr;
    }
    
    std::unique_ptr<StatementNode> result = nullptr;
    
    if (check(TokenType::tkn_LBRACE)) {
        result = parseBlockStmt();
    } else if (check(TokenType::tkn_IF)) {
        result = parseIfStmt();
    } else if (check(TokenType::tkn_WHILE)) {
        result = parseWhileStmt();
    } else if (check(TokenType::tkn_FOR)) {
        result = parseForStmt();
    } else if (check(TokenType::tkn_RETURN)) {
        result = parseReturnStmt();
    } else if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
                      TokenType::tkn_BOOL, TokenType::tkn_VOID,
                      TokenType::tkn_STRUCT})) {
        result = parseVarDecl();
    } else if (check(TokenType::tkn_IDENTIFIER)) {
        if (current + 1 < tokens.size() && 
            tokens[current + 1].type == TokenType::tkn_IDENTIFIER) {
            result = parseVarDecl();
        } else {
            result = parseExprStmt();
        }
    } else if (check({TokenType::tkn_INT_LITERAL, TokenType::tkn_FLOAT_LITERAL,
                      TokenType::tkn_STRING_LITERAL, TokenType::tkn_TRUE,
                      TokenType::tkn_FALSE, TokenType::tkn_LPAREN,
                      TokenType::tkn_INCREMENT, TokenType::tkn_DECREMENT,
                      TokenType::tkn_NOT, TokenType::tkn_SUBTRACTION})) {
        result = parseExprStmt();
    } else if (check(TokenType::tkn_SEMICOLON)) {
        result = parseEmptyStmt();
    } else {
        error("Ожидалась инструкция");
        if (!advanced_synchronize()) {
            result = nullptr;
        }
    }
    
    recursion_depth--;
    return result;
}

std::unique_ptr<StatementNode> Parser::parseBlockStmt() {
    return parseBlock();
}

std::unique_ptr<StatementNode> Parser::parseIfStmt() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе if");
        recursion_depth--;
        return nullptr;
    }
    
    Token ifToken = previous();
    if (ifToken.type != TokenType::tkn_IF) {
        ifToken = advance();
    }
    
    consume(TokenType::tkn_LPAREN, "Ожидалась '(' после 'if'");
    auto condition = parseExpression();
    consume(TokenType::tkn_RPAREN, "Ожидалась ')' после условия");
    
    auto thenBranch = parseStatement();
    auto elseBranch = parseElsePart();
    
    recursion_depth--;
    return std::make_unique<IfStmtNode>(
        ifToken.line, ifToken.column,
        std::move(condition),
        std::move(thenBranch),
        std::move(elseBranch)
    );
}

std::unique_ptr<StatementNode> Parser::parseElsePart() {
    if (match(TokenType::tkn_ELSE)) {
        return parseStatement();
    }
    return nullptr;
}

std::unique_ptr<StatementNode> Parser::parseWhileStmt() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе while");
        recursion_depth--;
        return nullptr;
    }
    
    Token whileToken = previous();
    if (whileToken.type != TokenType::tkn_WHILE) {
        whileToken = advance();
    }
    
    consume(TokenType::tkn_LPAREN, "Ожидалась '(' после 'while'");
    auto condition = parseExpression();
    consume(TokenType::tkn_RPAREN, "Ожидалась ')' после условия");
    
    auto body = parseStatement();
    
    recursion_depth--;
    return std::make_unique<WhileStmtNode>(
        whileToken.line, whileToken.column,
        std::move(condition),
        std::move(body)
    );
}

std::unique_ptr<StatementNode> Parser::parseForStmt() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе for");
        recursion_depth--;
        return nullptr;
    }
    
    Token forToken = previous();
    if (forToken.type != TokenType::tkn_FOR) {
        forToken = advance();
    }
    
    consume(TokenType::tkn_LPAREN, "Ожидалась '(' после 'for'");
    
    std::unique_ptr<StatementNode> init = nullptr;
    
    if (check({TokenType::tkn_INT, TokenType::tkn_FLOAT, 
               TokenType::tkn_BOOL, TokenType::tkn_VOID,
               TokenType::tkn_STRUCT})) {
        init = parseVarDecl();
    } else if (check(TokenType::tkn_IDENTIFIER)) {
        if (current + 1 < tokens.size() && 
            tokens[current + 1].type == TokenType::tkn_IDENTIFIER) {
            init = parseVarDecl();
        } else {
            auto expr = parseExpression();
            init = std::make_unique<ExprStmtNode>(
                expr->getLine(), expr->getColumn(),
                std::move(expr)
            );
            if (!check(TokenType::tkn_SEMICOLON)) {
                error("Ожидалась ';' после выражения в инициализации for");
            } else {
                advance();
            }
        }
    } else if (check({TokenType::tkn_INT_LITERAL, TokenType::tkn_FLOAT_LITERAL,
                      TokenType::tkn_STRING_LITERAL, TokenType::tkn_TRUE,
                      TokenType::tkn_FALSE, TokenType::tkn_LPAREN,
                      TokenType::tkn_INCREMENT, TokenType::tkn_DECREMENT,
                      TokenType::tkn_NOT, TokenType::tkn_SUBTRACTION})) {
        auto expr = parseExpression();
        init = std::make_unique<ExprStmtNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr)
        );
        if (!check(TokenType::tkn_SEMICOLON)) {
            error("Ожидалась ';' после выражения в инициализации for");
        } else {
            advance();
        }
    } else {
        consume(TokenType::tkn_SEMICOLON, "Ожидалась ';' в инициализации for");
    }
    
    std::unique_ptr<ExpressionNode> condition = nullptr;
    if (!check(TokenType::tkn_SEMICOLON)) {
        condition = parseExpression();
    }
    consume(TokenType::tkn_SEMICOLON, "Ожидалась ';' после условия цикла for");
    
    std::unique_ptr<ExpressionNode> update = nullptr;
    if (!check(TokenType::tkn_RPAREN)) {
        update = parseExpression();
    }
    consume(TokenType::tkn_RPAREN, "Ожидалась ')' после заголовка цикла for");
    
    auto body = parseStatement();
    
    recursion_depth--;
    return std::make_unique<ForStmtNode>(
        forToken.line, forToken.column,
        std::move(init),
        std::move(condition),
        std::move(update),
        std::move(body)
    );
}

std::unique_ptr<StatementNode> Parser::parseReturnStmt() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе return");
        recursion_depth--;
        return nullptr;
    }
    
    Token returnToken = previous();
    if (returnToken.type != TokenType::tkn_RETURN) {
        returnToken = advance();
    }
    
    std::unique_ptr<ExpressionNode> value = nullptr;
    if (!check(TokenType::tkn_SEMICOLON)) {
        value = parseExpression();
    }
    
    if (check(TokenType::tkn_SEMICOLON)) {
        advance();
    } else if (!check(TokenType::tkn_RBRACE) && !isAtEnd()) {
        error("Ожидалась ';' после return");
        
        if (!in_error_recovery) {
            in_error_recovery = true;
            int recover_iterations = 0;
            while (!isAtEnd() && !check(TokenType::tkn_SEMICOLON) && 
                   !check(TokenType::tkn_RBRACE) && recover_iterations < 100) {
                advance();
                recover_iterations++;
            }
            if (check(TokenType::tkn_SEMICOLON)) {
                advance();
                metrics.mark_recovered(RecoveryType::PANIC_MODE);
            }
            in_error_recovery = false;
        }
    }
    
    recursion_depth--;
    return std::make_unique<ReturnStmtNode>(
        returnToken.line, returnToken.column,
        std::move(value)
    );
}

std::unique_ptr<StatementNode> Parser::parseExprStmt() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе выражения-инструкции");
        recursion_depth--;
        return nullptr;
    }
    
    auto expr = parseExpression();
    
    if (check(TokenType::tkn_SEMICOLON)) {
        advance();
    } else if (!check(TokenType::tkn_RBRACE) && !isAtEnd()) {
        error("Ожидалась ';' после выражения");
        
        if (!in_error_recovery) {
            in_error_recovery = true;
            int recover_iterations = 0;
            while (!isAtEnd() && !check(TokenType::tkn_SEMICOLON) && 
                   !check(TokenType::tkn_RBRACE) && recover_iterations < 100) {
                advance();
                recover_iterations++;
            }
            if (check(TokenType::tkn_SEMICOLON)) {
                advance();
                metrics.mark_recovered(RecoveryType::PANIC_MODE);
            }
            in_error_recovery = false;
        }
    }
    
    recursion_depth--;
    return std::make_unique<ExprStmtNode>(
        expr->getLine(), expr->getColumn(),
        std::move(expr)
    );
}

std::unique_ptr<StatementNode> Parser::parseEmptyStmt() {
    Token semicolon = advance();
    return std::make_unique<ExprStmtNode>(
        semicolon.line, semicolon.column,
        nullptr
    );
}

std::unique_ptr<BlockStmtNode> Parser::parseBlock() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе блока");
        recursion_depth--;
        return nullptr;
    }
    
    Token lbrace;
    if (previous().type == TokenType::tkn_LBRACE) {
        lbrace = previous();
    } else if (check(TokenType::tkn_LBRACE)) {
        lbrace = advance();
    } else {
        error("Ожидалась '{' в начале блока");
        recursion_depth--;
        return nullptr;
    }
    
    auto block = std::make_unique<BlockStmtNode>(lbrace.line, lbrace.column);
    
    int iterations = 0;
    while (!check(TokenType::tkn_RBRACE) && !isAtEnd() && iterations < max_iterations) {
        iterations++;
        
        if (check(TokenType::tkn_SEMICOLON)) {
            advance();
            continue;
        }
        
        auto stmt = parseStatement();
        if (stmt) {
            block->addStatement(std::move(stmt));
        } else {
            if (!advanced_synchronize()) break;
        }
    }
    
    if (isAtEnd()) {
        error("Неожиданный конец файла - отсутствует закрывающая скобка '}'", lbrace);
        recursion_depth--;
        return block;
    }
    
    consume(TokenType::tkn_RBRACE, "Ожидалась '}' после блока");
    
    recursion_depth--;
    return block;
}

std::unique_ptr<ExpressionNode> Parser::parseExpression() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе выражения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto result = parseAssignment();
    
    recursion_depth--;
    return result;
}

std::unique_ptr<ExpressionNode> Parser::parseAssignment() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе присваивания");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseLogicalOr();
    
    if (match({TokenType::tkn_ASSIGNMENT,
               TokenType::tkn_ASSIGNMENT_AFTER_ADDITION,
               TokenType::tkn_ASSIGNMENT_AFTER_SUBTRACTION,
               TokenType::tkn_ASSIGNMENT_AFTER_MULTIPLICATION,
               TokenType::tkn_ASSIGNMENT_AFTER_SEGMENTATION,
               TokenType::tkn_ASSIGNMENT_AFTER_MODULO})) {
        
        TokenType op = previous().type;
        auto value = parseAssignment();
        
        recursion_depth--;
        return std::make_unique<AssignmentExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(value)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalOr() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе logical or");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseLogicalAnd();
    
    int iterations = 0;
    while (match(TokenType::tkn_OR) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseLogicalAnd() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе logical and");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseEquality();
    
    int iterations = 0;
    while (match(TokenType::tkn_AND) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseEquality() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе равенства");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseRelational();
    
    int iterations = 0;
    while (match({TokenType::tkn_EQUAL, TokenType::tkn_NOT_EQUAL}) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseRelational();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseRelational() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе отношения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseAdditive();
    
    int iterations = 0;
    while (match({TokenType::tkn_LESS, TokenType::tkn_MORE,
                  TokenType::tkn_LESS_OR_EQUAL, TokenType::tkn_MORE_OR_EQUAL}) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseAdditive() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе сложения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseMultiplicative();
    
    int iterations = 0;
    while (match({TokenType::tkn_ADDITION, TokenType::tkn_SUBTRACTION}) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseMultiplicative() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе умножения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parseUnary();
    
    int iterations = 0;
    while (match({TokenType::tkn_MULTIPLICATION, TokenType::tkn_SEGMENTATION,
                  TokenType::tkn_MODULO}) && iterations < max_iterations) {
        iterations++;
        TokenType op = previous().type;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExprNode>(
            expr->getLine(), expr->getColumn(),
            std::move(expr), op, std::move(right)
        );
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parseUnary() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе унарного выражения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    if (match({TokenType::tkn_SUBTRACTION, TokenType::tkn_NOT,
               TokenType::tkn_INCREMENT, TokenType::tkn_DECREMENT})) {
        TokenType op = previous().type;
        auto operand = parseUnary();
        recursion_depth--;
        return std::make_unique<UnaryExprNode>(
            operand->getLine(), operand->getColumn(),
            op, std::move(operand)
        );
    }
    
    auto expr = parsePostfix();
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parsePostfix() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе постфиксного выражения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    auto expr = parsePrimary();
    
    int iterations = 0;
    while (iterations < max_iterations) {
        iterations++;
        
        if (match(TokenType::tkn_LPAREN)) {
            auto call = std::make_unique<CallExprNode>(
                expr->getLine(), expr->getColumn(),
                std::move(expr)
            );
            
            if (!check(TokenType::tkn_RPAREN)) {
                auto args = parseArgumentList();
                for (auto& arg : args) {
                    call->addArgument(std::move(arg));
                }
            }
            
            consume(TokenType::tkn_RPAREN, "Ожидалась ')' после аргументов");
            expr = std::move(call);
        } else if (match(TokenType::tkn_POINT)) {
            Token dotToken = previous();
            Token fieldToken = consume(TokenType::tkn_IDENTIFIER, "Ожидалось имя поля после '.'");
            
            auto field = std::make_unique<IdentifierExprNode>(
                fieldToken.line, fieldToken.column, fieldToken.lexeme
            );
            
            expr = std::make_unique<BinaryExprNode>(
                dotToken.line, dotToken.column,
                std::move(expr), TokenType::tkn_POINT, std::move(field)
            );
        } else if (match(TokenType::tkn_INCREMENT)) {
            expr = std::make_unique<UnaryExprNode>(
                expr->getLine(), expr->getColumn(),
                TokenType::tkn_INCREMENT, std::move(expr)
            );
        } else if (match(TokenType::tkn_DECREMENT)) {
            expr = std::make_unique<UnaryExprNode>(
                expr->getLine(), expr->getColumn(),
                TokenType::tkn_DECREMENT, std::move(expr)
            );
        } else {
            break;
        }
    }
    
    recursion_depth--;
    return expr;
}

std::unique_ptr<ExpressionNode> Parser::parsePrimary() {
    recursion_depth++;
    if (recursion_depth > max_recursion_depth) {
        error("Превышена максимальная глубина рекурсии при разборе первичного выражения");
        recursion_depth--;
        return std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    std::unique_ptr<ExpressionNode> result = nullptr;
    
    if (match(TokenType::tkn_IDENTIFIER)) {
        Token token = previous();
        result = std::make_unique<IdentifierExprNode>(
            token.line, token.column, token.lexeme
        );
    } else if (match(TokenType::tkn_INT_LITERAL)) {
        Token token = previous();
        if (auto val = token.getValue<int>()) {
            result = std::make_unique<LiteralExprNode>(
                token.line, token.column, *val
            );
        }
    } else if (match(TokenType::tkn_FLOAT_LITERAL)) {
        Token token = previous();
        if (auto val = token.getValue<double>()) {
            result = std::make_unique<LiteralExprNode>(
                token.line, token.column, *val
            );
        }
    } else if (match(TokenType::tkn_STRING_LITERAL)) {
        Token token = previous();
        if (auto val = token.getValue<std::string>()) {
            result = std::make_unique<LiteralExprNode>(
                token.line, token.column, *val
            );
        }
    } else if (match(TokenType::tkn_TRUE)) {
        Token token = previous();
        result = std::make_unique<LiteralExprNode>(
            token.line, token.column, true
        );
    } else if (match(TokenType::tkn_FALSE)) {
        Token token = previous();
        result = std::make_unique<LiteralExprNode>(
            token.line, token.column, false
        );
    } else if (match(TokenType::tkn_LPAREN)) {
        auto expr = parseExpression();
        if (!check(TokenType::tkn_RPAREN)) {
            error("Ожидалась ')' после выражения");
        } else {
            advance();
        }
        result = std::move(expr);
    } else {
        error("Ожидалось выражение");
        result = std::make_unique<LiteralExprNode>(0, 0, 0);
    }
    
    recursion_depth--;
    return result;
}

std::vector<std::unique_ptr<ExpressionNode>> Parser::parseArgumentList() {
    std::vector<std::unique_ptr<ExpressionNode>> args;
    
    if (!check(TokenType::tkn_RPAREN)) {
        auto arg = parseExpression();
        if (arg) {
            args.push_back(std::move(arg));
        }
        
        int iterations = 0;
        while (match(TokenType::tkn_COMMA) && iterations < max_iterations) {
            iterations++;
            if (!check(TokenType::tkn_RPAREN)) {
                arg = parseExpression();
                if (arg) {
                    args.push_back(std::move(arg));
                }
            } else {
                break;
            }
        }
    }
    
    return args;
}

bool Parser::hasErrors() const { return hadError; }

const std::vector<ParseError>& Parser::getErrors() const { return errors; }

const ErrorMetrics& Parser::getMetrics() const { return metrics; }

const std::vector<RecoverySuggestion>& Parser::getSuggestions() const { return suggestions; }

void Parser::setMaxErrorCount(int max) { metrics.max_error_count = max; }
