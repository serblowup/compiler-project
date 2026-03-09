#include "lexer.hpp"
#include <cctype>
#include <algorithm>
#include <sstream>

// Инициализация структуры лексера
Lexer::Lexer(const std::string& source) 
    : source(source), line(1), column(1), start_line(1), start_column(1) {
    start = current = source.begin();
}

// Продвижение лексера на один символ вперед
char Lexer::lexer_advance() {
    if (lexer_is_at_end()) return '\0';
    char c = *current;
    current++;

    if (c == '\n') {
        line++;
        column = 1;
    } else {
        column++;
    }

    return c;
}

// Просмотр текущего символа без продвижения
char Lexer::lexer_peek() const {
    return lexer_is_at_end() ? '\0' : *current;
}

// Просмотр следующего символа без продвижения
char Lexer::lexer_peek_next() const {
    if (lexer_is_at_end()) return '\0';
    if (current + 1 == source.end()) return '\0';
    return *(current + 1);
}

// Проверка и потребление символа, если он совпадает с ожидаемым
bool Lexer::lexer_match(char expected) {
    if (lexer_is_at_end()) return false;
    if (*current != expected) return false;
    lexer_advance();
    return true;
}

// Создание токена заданного типа
Token Lexer::lexer_make_token(TokenType type) {
    Token token;
    token.type = type;
    
    // Извлекаем лексему
    token.lexeme = std::string(start, current);
    
    // Используем сохраненные позиции начала токена
    token.line = start_line;
    token.column = start_column;
    
    token.literal_type = LiteralType::LITERAL_NONE;
    
    return token;
}

// Создание токена ошибки с сообщением
Token Lexer::lexer_make_error(const std::string& message) {
    Token token;
    token.type = TokenType::TOKEN_ERROR;
    token.lexeme = "ERROR: [Lexer] " + message;
    token.line = start_line;
    token.column = start_column;
    token.literal_type = LiteralType::LITERAL_NONE;
    return token;
}

// Распознавание числовых литералов (целых и с плавающей точкой)
Token Lexer::lexer_make_number() {
    // Сначала собираем целую часть
    while (lexer_is_digit(lexer_peek())) {
        lexer_advance();
    }

    // Проверяем, есть ли дробная часть
    if (lexer_peek() == '.' && lexer_is_digit(lexer_peek_next())) {
        // Это число с плавающей точкой - пропускаем точку
        lexer_advance();

        // Собираем дробную часть
        while (lexer_is_digit(lexer_peek())) {
            lexer_advance();
        }

        // Создаем токен для float
        Token token = lexer_make_token(TokenType::tkn_FLOAT_LITERAL);

        // Извлекаем значение
        std::string num_str(start, current);
        try {
            double value = std::stod(num_str);
            token.value = value;
            token.literal_type = LiteralType::LITERAL_FLOAT;
        } catch (...) {
            // В случае ошибки преобразования создаем токен ошибки
            return lexer_make_error("Некорректное число с плавающей точкой: " + num_str);
        }
        
        return token;
    } else {
        // Это целое число
        Token token = lexer_make_token(TokenType::tkn_INT_LITERAL);

        // Извлекаем значение
        std::string num_str(start, current);
        try {
            int value = std::stoi(num_str);
            token.value = value;
            token.literal_type = LiteralType::LITERAL_INT;
        } catch (...) {
            // В случае ошибки преобразования создаем токен ошибки
            return lexer_make_error("Некорректное целое число: " + num_str);
        }
        
        return token;
    }
}

// Распознавание строковых литералов
Token Lexer::lexer_make_string() {
    // Пропускаем открывающую кавычку - начало строки уже установлено на '"'
    // Собираем содержимое строки
    while (!lexer_is_at_end() && lexer_peek() != '"') {
        if (lexer_peek() == '\\') {
            // Пропускаем обратный слэш
            lexer_advance();
        }
        lexer_advance();
    }

    // Проверяем, не конец ли файла
    if (lexer_is_at_end()) {
        return lexer_make_error("Незавершенная строка");
    }

    // Пропускаем закрывающую кавычку
    lexer_advance();

    // Создаем токен
    Token token = lexer_make_token(TokenType::tkn_STRING_LITERAL);

    // Извлекаем содержимое строки (без кавычек)
    std::string str_value(start + 1, current - 1);
    
    token.value = str_value;
    token.literal_type = LiteralType::LITERAL_STRING;

    return token;
}

// Распознавание идентификаторов и ключевых слов
Token Lexer::lexer_make_identifier() {
    // Собираем все символы идентификатора
    while (lexer_is_alnum(lexer_peek())) {
        lexer_advance();
    }

    // Определяем, является ли это ключевым словом (по умолчанию - идентификатор)
    std::string identifier(start, current);
    TokenType type = TokenType::tkn_IDENTIFIER;

    // Проверка ключевых слов
    if (identifier == "if") type = TokenType::tkn_IF;
    else if (identifier == "else") type = TokenType::tkn_ELSE;
    else if (identifier == "while") type = TokenType::tkn_WHILE;
    else if (identifier == "for") type = TokenType::tkn_FOR;
    else if (identifier == "int") type = TokenType::tkn_INT;
    else if (identifier == "float") type = TokenType::tkn_FLOAT;
    else if (identifier == "bool") type = TokenType::tkn_BOOL;
    else if (identifier == "return") type = TokenType::tkn_RETURN;
    else if (identifier == "true") {
        // Для true устанавливаем литеральное значение
        Token token = lexer_make_token(TokenType::tkn_TRUE);
        token.value = true;
        token.literal_type = LiteralType::LITERAL_BOOL;
        return token;
    }
    else if (identifier == "false") {
        // Для false устанавливаем литеральное значение
        Token token = lexer_make_token(TokenType::tkn_FALSE);
        token.value = false;
        token.literal_type = LiteralType::LITERAL_BOOL;
        return token;
    }
    else if (identifier == "void") type = TokenType::tkn_VOID;
    else if (identifier == "struct") type = TokenType::tkn_STRUCT;
    else if (identifier == "fn") type = TokenType::tkn_FN;

    // Создаем токен
    return lexer_make_token(type);
}

// Проверка, является ли символ цифрой
bool Lexer::lexer_is_digit(char c) {
    return c >= '0' && c <= '9';
}

// Проверка, является ли символ буквой или подчеркиванием
bool Lexer::lexer_is_alpha(char c) {
    return (c >= 'a' && c <= 'z') ||
           (c >= 'A' && c <= 'Z') ||
           c == '_';
}

// Проверка, является ли символ буквой, цифрой или подчеркиванием
bool Lexer::lexer_is_alnum(char c) {
    return lexer_is_alpha(c) || lexer_is_digit(c);
}

// Пропуск пробельных символов и переводов строк
void Lexer::lexer_skip_whitespace() {
    while (true) {
        char c = lexer_peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            lexer_advance(); // lexer_advance сам обрабатывает увеличение line при '\n'
        } else {
            break;
        }
    }
}

// Пропуск многострочных комментариев
void Lexer::lexer_skip_comment() {
    while (!lexer_is_at_end()) {
        // Пропускаем "*" и "/"
        if (lexer_peek() == '*' && lexer_peek_next() == '/') {
            lexer_advance();
            lexer_advance();
            break;
        }
        lexer_advance();
    }
}

// Получение следующего токена из исходного
Token Lexer::lexer_next_token() {
    // Пропуск пробелов
    lexer_skip_whitespace();

    // Устанавливаем начало нового токена
    start = current;
    start_line = line;      // Сохраняем строку начала токена
    start_column = column;

    if (lexer_is_at_end()) {
        Token eof_token;
        eof_token.type = TokenType::TOKEN_END_OF_FILE;
        eof_token.lexeme = "";
        eof_token.line = start_line;      // Используем сохраненную строку
        eof_token.column = start_column;  // Используем сохраненную колонку
        eof_token.literal_type = LiteralType::LITERAL_NONE;
        return eof_token;
    }

    // Получаем символ
    char c = *current;

    // Проверяем категории символов
    if (lexer_is_alpha(c)) {
        return lexer_make_identifier();
    }

    if (lexer_is_digit(c)) {
        return lexer_make_number();
    }

    // Для остальных символов получаем и продвигаем
    c = *current;
    lexer_advance();

    switch (c) {
        // Разделители
        case '(': return lexer_make_token(TokenType::tkn_LPAREN);
        case ')': return lexer_make_token(TokenType::tkn_RPAREN);
        case '{': return lexer_make_token(TokenType::tkn_LBRACE);
        case '}': return lexer_make_token(TokenType::tkn_RBRACE);
        case '[': return lexer_make_token(TokenType::tkn_LBRACKET);
        case ']': return lexer_make_token(TokenType::tkn_RBRACKET);
        case ';': return lexer_make_token(TokenType::tkn_SEMICOLON);
        case ',': return lexer_make_token(TokenType::tkn_COMMA);
        case '.': return lexer_make_token(TokenType::tkn_POINT);
        case ':': return lexer_make_token(TokenType::tkn_COLON);

        // Строковый литерал
        case '"': return lexer_make_string();

        // Проверка символа "/" - комментарий или оператор
        case '/':
            // Если "//" - однострочный комментарий и пропуск всех символов до конца строки
            if (lexer_match('/')) {
                while (lexer_peek() != '\n' && !lexer_is_at_end()) {
                    lexer_advance();
                }
                // Не пропускаем символ новой строки здесь - пусть будет обработан в следующем вызове
                return lexer_next_token();
            // Если "/*" - многострочный комментарий
            } else if (lexer_match('*')) {
                lexer_skip_comment();
                return lexer_next_token();
            // Если "/=" - оператор "Присваивание после деления"
            } else if (lexer_match('=')) {
                return lexer_make_token(TokenType::tkn_ASSIGNMENT_AFTER_SEGMENTATION);
            // Если "/" - оператор "Деление"
            } else {
                return lexer_make_token(TokenType::tkn_SEGMENTATION);
            }

        // Проверка символа "+" - "Инкремент" или "Присваивание после сложения" или "Сложение"
        case '+':
            if (lexer_match('+')) return lexer_make_token(TokenType::tkn_INCREMENT);
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_ASSIGNMENT_AFTER_ADDITION);
            return lexer_make_token(TokenType::tkn_ADDITION);

        // Проверка символа "-" - "Стрелка", "Декремент", "Присваивание после вычитания" или "Вычитание"
        case '-':
            if (lexer_match('>')) return lexer_make_token(TokenType::tkn_ARROW);
            if (lexer_match('-')) return lexer_make_token(TokenType::tkn_DECREMENT);
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_ASSIGNMENT_AFTER_SUBTRACTION);
            return lexer_make_token(TokenType::tkn_SUBTRACTION);

        // Проверка символа "*" - "Присваивание после умножения" или "Умножение"
        case '*':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_ASSIGNMENT_AFTER_MULTIPLICATION);
            return lexer_make_token(TokenType::tkn_MULTIPLICATION);

        // Проверка символа "%" - "Присваивание после деления по модулю" или "Деление по модулю"
        case '%':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_ASSIGNMENT_AFTER_MODULO);
            return lexer_make_token(TokenType::tkn_MODULO);

        // Проверка символа "!" - "Не равно" или "Логическое не"
        case '!':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_NOT_EQUAL);
            return lexer_make_token(TokenType::tkn_NOT);

        // Проверка символа "=" - "Равно" или "Присваивание"
        case '=':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_EQUAL);
            return lexer_make_token(TokenType::tkn_ASSIGNMENT);

        // Проверка символа "<" - "Меньше или равно" или "Меньше"
        case '<':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_LESS_OR_EQUAL);
            return lexer_make_token(TokenType::tkn_LESS);

        // Проверка символа ">" - "Больше или равно" или "Больше"
        case '>':
            if (lexer_match('=')) return lexer_make_token(TokenType::tkn_MORE_OR_EQUAL);
            return lexer_make_token(TokenType::tkn_MORE);

        // Проверка символа "&" - "Логическое и", одиночный символ "&" недопустим!
        case '&':
            if (lexer_match('&')) return lexer_make_token(TokenType::tkn_AND);
            break;

        // Проверка символа "|" - "Логическое или", одиночный символ "|" недопустим!
        case '|':
            if (lexer_match('|')) return lexer_make_token(TokenType::tkn_OR);
            break;
    }
    
    // Токен ошибки, если символ не распознан
    return lexer_make_error("Недопустимый символ!!!");
}

// Освобождение памяти токена (оставлено для совместимости)
void Lexer::token_free(Token* token) {
    if (token) {
    }
}

bool Lexer::lexer_is_at_end() const {
    return current == source.end() || *current == '\0';
}

Token Lexer::lexer_peek_token() {
    // Создаем копию лексера для просмотра вперед
    Lexer copy = *this;
    return copy.lexer_next_token();
}

int Lexer::lexer_get_line() const { 
    return line; 
}

int Lexer::lexer_get_column() const { 
    return column; 
}