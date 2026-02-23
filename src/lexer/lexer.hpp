#ifndef LEXER_H
#define LEXER_H

#include "tokens.hpp"
#include <string>

/* Структура лексера:
 * - Исходный код;
 * - Начало текущей лексемы;
 * - Текущая позиция в исходном коде;
 * - Текущая строка;
 * - Текущая колонка.
*/
class Lexer {
private:
    std::string source;
    std::string::const_iterator start;
    std::string::const_iterator current;
    int line;
    int column;
    int start_line;
    int start_column;

    // Вспомогательные методы
    char lexer_advance();
    char lexer_peek() const;
    char lexer_peek_next() const;
    bool lexer_match(char expected);
    
    // Создание токенов
    Token lexer_make_token(TokenType type);
    Token lexer_make_error(const std::string& message);
    Token lexer_make_number();
    Token lexer_make_string();
    Token lexer_make_identifier();
    
    // Проверки символов
    static bool lexer_is_digit(char c);
    static bool lexer_is_alpha(char c);
    static bool lexer_is_alnum(char c);
    
    // Пропуск пробелов и комментариев
    void lexer_skip_whitespace();
    void lexer_skip_comment();

public:
    // Инициализация лексера
    Lexer(const std::string& source);
    
    // Освобождение памяти токена
    static void token_free(Token* token);
    
    // Получение следующего токена
    Token lexer_next_token();
    
    // Просмотр следующего токена без продвижения
    Token lexer_peek_token();
    
    // Проверка конца файла
    bool lexer_is_at_end() const;
    
    // Получение текущей позиции
    int lexer_get_line() const;
    int lexer_get_column() const;
};

#endif