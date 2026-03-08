#ifndef TOKENS_H
#define TOKENS_H

#include <string>
#include <variant>
#include <optional>

// Типы токенов
enum class TokenType {
    /* Ключевые слова:
     * - if;
     * - else;
     * - while;
     * - for;
     * - int;
     * - float;
     * - bool;
     * - return;
     * - true;
     * - false;
     * - void;
     * - struct;
     * - fn.
     */
    tkn_IF,
    tkn_ELSE,
    tkn_WHILE,
    tkn_FOR,
    tkn_INT,
    tkn_FLOAT,
    tkn_BOOL,
    tkn_RETURN,
    tkn_TRUE,
    tkn_FALSE,
    tkn_VOID,
    tkn_STRUCT,
    tkn_FN,

    // Идентификатор
    tkn_IDENTIFIER,

    /* Типы литералов:
     * - Целые числа (Integer);
     * - Числа с плавающей точкой (Float);
     * - Строки (String);
     * - Логические (Boolean).
     */
    tkn_INT_LITERAL,
    tkn_FLOAT_LITERAL,
    tkn_STRING_LITERAL,
    tkn_BOOLEAN_LITERAL,

    /* Арифметические операторы:
     * - Сложение;
     * - Вычитание;
     * - Умножение;
     * - Деление;
     * - Взятие остатка.
     */
    tkn_ADDITION,
    tkn_SUBTRACTION,
    tkn_MULTIPLICATION,
    tkn_SEGMENTATION,
    tkn_MODULO,

    /* Операторы отношения:
     * - Равно;
     * - Не равно;
     * - Больше;
     * - Меньше;
     * - Больше или равно;
     * - Меньше или равно.
     */
    tkn_EQUAL,
    tkn_NOT_EQUAL,
    tkn_MORE,
    tkn_LESS,
    tkn_MORE_OR_EQUAL,
    tkn_LESS_OR_EQUAL,

    /* Логические операторы:
     * - Логическое и;
     * - Логическое или;
     * - Логическое не.
     */
    tkn_AND,
    tkn_OR,
    tkn_NOT,

    /* Операторы присваивания:
     * - Присваивание;
     * - Присваивание после сложения;
     * - Присваивание после вычитания;
     * - Присваивание после умножения;
     * - Присваивание после деления;
     * - Присваивание после деления по модулю.
     */
    tkn_ASSIGNMENT,
    tkn_ASSIGNMENT_AFTER_ADDITION,
    tkn_ASSIGNMENT_AFTER_SUBTRACTION,
    tkn_ASSIGNMENT_AFTER_MULTIPLICATION,
    tkn_ASSIGNMENT_AFTER_SEGMENTATION,
    tkn_ASSIGNMENT_AFTER_MODULO,

    /* Прочие операторы:
     * - Инкремент;
     * - Декремент.
     */
    tkn_INCREMENT,
    tkn_DECREMENT,

    /* Разделители:
     * - Точка с запятой;
     * - Запятая;
     * - Точка;
     * - Двоеточие;
     * - Левая круглая скобка;
     * - Правая круглая скобка;
     * - Левая фигурная скобка;
     * - Правая фигурная скобка;
     * - Левая квадратная скобка;
     * - Правая квадратная скобка.
     */
    tkn_SEMICOLON,
    tkn_COMMA,
    tkn_POINT,
    tkn_COLON,
    tkn_LPAREN,
    tkn_RPAREN,
    tkn_LBRACE,
    tkn_RBRACE,
    tkn_LBRACKET,
    tkn_RBRACKET,

    // Специальные токены
    TOKEN_END_OF_FILE,
    TOKEN_ERROR
};

// Тип литерала для доступа к variant
enum class LiteralType {
    LITERAL_NONE,
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_STRING,
    LITERAL_BOOL
};

// Функция преобразования типа токена в строковое представление
inline const char* token_type_to_string(TokenType type) {
    switch (type) {
        // Ключевые слова
        case TokenType::tkn_IF: return "KW_IF";
        case TokenType::tkn_ELSE: return "KW_ELSE";
        case TokenType::tkn_WHILE: return "KW_WHILE";
        case TokenType::tkn_FOR: return "KW_FOR";
        case TokenType::tkn_INT: return "KW_INT";
        case TokenType::tkn_FLOAT: return "KW_FLOAT";
        case TokenType::tkn_BOOL: return "KW_BOOL";
        case TokenType::tkn_RETURN: return "KW_RETURN";
        case TokenType::tkn_TRUE: return "KW_TRUE";
        case TokenType::tkn_FALSE: return "KW_FALSE";
        case TokenType::tkn_VOID: return "KW_VOID";
        case TokenType::tkn_STRUCT: return "KW_STRUCT";
        case TokenType::tkn_FN: return "KW_FN";
        
        // Идентификатор
        case TokenType::tkn_IDENTIFIER: return "IDENTIFIER";
        
        // Литералы
        case TokenType::tkn_INT_LITERAL: return "INT_LITERAL";
        case TokenType::tkn_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TokenType::tkn_STRING_LITERAL: return "STRING_LITERAL";
        case TokenType::tkn_BOOLEAN_LITERAL: return "BOOLEAN_LITERAL";
        
        // Арифметические операторы
        case TokenType::tkn_ADDITION: return "ADD";
        case TokenType::tkn_SUBTRACTION: return "SUB";
        case TokenType::tkn_MULTIPLICATION: return "MUL";
        case TokenType::tkn_SEGMENTATION: return "DIV";
        case TokenType::tkn_MODULO: return "MOD";
        
        // Операторы отношения
        case TokenType::tkn_EQUAL: return "EQ";
        case TokenType::tkn_NOT_EQUAL: return "NEQ";
        case TokenType::tkn_MORE: return "GT";
        case TokenType::tkn_LESS: return "LT";
        case TokenType::tkn_MORE_OR_EQUAL: return "GTE";
        case TokenType::tkn_LESS_OR_EQUAL: return "LTE";
        
        // Логические операторы
        case TokenType::tkn_AND: return "AND";
        case TokenType::tkn_OR: return "OR";
        case TokenType::tkn_NOT: return "NOT";
        
        // Операторы присваивания
        case TokenType::tkn_ASSIGNMENT: return "ASSIGN";
        case TokenType::tkn_ASSIGNMENT_AFTER_ADDITION: return "ADD_ASSIGN";
        case TokenType::tkn_ASSIGNMENT_AFTER_SUBTRACTION: return "SUB_ASSIGN";
        case TokenType::tkn_ASSIGNMENT_AFTER_MULTIPLICATION: return "MUL_ASSIGN";
        case TokenType::tkn_ASSIGNMENT_AFTER_SEGMENTATION: return "DIV_ASSIGN";
        case TokenType::tkn_ASSIGNMENT_AFTER_MODULO: return "MOD_ASSIGN";
        
        // Инкремент/Декремент
        case TokenType::tkn_INCREMENT: return "INC";
        case TokenType::tkn_DECREMENT: return "DEC";
        
        // Разделители
        case TokenType::tkn_SEMICOLON: return "SEMICOLON";
        case TokenType::tkn_COMMA: return "COMMA";
        case TokenType::tkn_POINT: return "DOT";
        case TokenType::tkn_COLON: return "COLON";
        case TokenType::tkn_LPAREN: return "LPAREN";
        case TokenType::tkn_RPAREN: return "RPAREN";
        case TokenType::tkn_LBRACE: return "LBRACE";
        case TokenType::tkn_RBRACE: return "RBRACE";
        case TokenType::tkn_LBRACKET: return "LBRACKET";
        case TokenType::tkn_RBRACKET: return "RBRACKET";
        
        // Специальные
        case TokenType::TOKEN_END_OF_FILE: return "EOF";
        case TokenType::TOKEN_ERROR: return "ERROR";
        
        default: return "UNKNOWN";
    }
}

/* Структура токена:
 * - Тип токена;
 * - Лексема(строка);
 * - Номер строки (начиная с 1);
 * - Номер колонки (начиная с 1);
 * - Значение литерала;
 * - Тип литерала.
 */
class Token {
public:
    TokenType type;
    std::string lexeme;
    int line;
    int column;
    
    // Значение литерала хранится в variant
    std::variant<
        std::monostate,  // LITERAL_NONE
        int,            // LITERAL_INT
        double,         // LITERAL_FLOAT
        std::string,    // LITERAL_STRING
        bool            // LITERAL_BOOL
    > value;
    
    LiteralType literal_type;

    // Конструкторы
    Token() : type(TokenType::TOKEN_ERROR), line(0), column(0), 
              literal_type(LiteralType::LITERAL_NONE) {}
    
    Token(TokenType type, const std::string& lexeme, int line, int column)
        : type(type), lexeme(lexeme), line(line), column(column),
          literal_type(LiteralType::LITERAL_NONE) {}
    
    Token(TokenType type, const std::string& lexeme, int line, int column, 
          int int_value)
        : type(type), lexeme(lexeme), line(line), column(column),
          value(int_value), literal_type(LiteralType::LITERAL_INT) {}
    
    Token(TokenType type, const std::string& lexeme, int line, int column, 
          double float_value)
        : type(type), lexeme(lexeme), line(line), column(column),
          value(float_value), literal_type(LiteralType::LITERAL_FLOAT) {}
    
    Token(TokenType type, const std::string& lexeme, int line, int column, 
          const std::string& string_value)
        : type(type), lexeme(lexeme), line(line), column(column),
          value(string_value), literal_type(LiteralType::LITERAL_STRING) {}
    
    Token(TokenType type, const std::string& lexeme, int line, int column, 
          bool bool_value)
        : type(type), lexeme(lexeme), line(line), column(column),
          value(bool_value), literal_type(LiteralType::LITERAL_BOOL) {}

    // Метод для получения строкового представления токена
    std::string toString() const {
        std::string result = std::to_string(line) + ":" + std::to_string(column) + " ";
        
        // Добавляем тип токена
        result += token_type_to_string(type);
        result += " \"" + lexeme + "\"";
        
        // Добавляем значение литерала, если есть
        if (literal_type != LiteralType::LITERAL_NONE) {
            // Для true/false не выводим значение, так как тесты ожидают только KW_TRUE/KW_FALSE
            if (type != TokenType::tkn_TRUE && type != TokenType::tkn_FALSE) {
                result += " [";
                switch (literal_type) {
                    case LiteralType::LITERAL_INT:
                        result += std::to_string(std::get<int>(value));
                        break;
                    case LiteralType::LITERAL_FLOAT:
                        result += std::to_string(std::get<double>(value));
                        break;
                    case LiteralType::LITERAL_STRING:
                        result += std::get<std::string>(value);
                        break;
                    case LiteralType::LITERAL_BOOL:
                        result += std::get<bool>(value) ? "true" : "false";
                        break;
                    default:
                        break;
                }
                result += "]";
            }
        }
        
        return result;
    }
    
    // Проверка наличия значения
    bool hasValue() const {
        return literal_type != LiteralType::LITERAL_NONE;
    }
    
    // Безопасное получение значения
    template<typename T>
    std::optional<T> getValue() const {
        try {
            if (std::holds_alternative<T>(value)) {
                return std::get<T>(value);
            }
        } catch (...) {
            // Неверный тип
        }
        return std::nullopt;
    }
};

#endif