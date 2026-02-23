#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

// Типы директив препроцессора
enum class DirectiveType {
    DIRECTIVE_DEFINE,
    DIRECTIVE_UNDEF,
    DIRECTIVE_IFDEF,
    DIRECTIVE_IFNDEF,
    DIRECTIVE_ELSE,
    DIRECTIVE_ENDIF,
    DIRECTIVE_NONE
};

// Структура для хранения макроса
struct Macro {
    std::string name;
    std::string replacement;
    int line; // Строка определения для отладки
    
    Macro() : line(0) {}
    Macro(const std::string& n, const std::string& r, int l) 
        : name(n), replacement(r), line(l) {}
};

// Состояние условной компиляции
struct ConditionalState {
    bool is_active; // Активна ли текущая ветка
    bool was_true;  // Была ли уже истинная ветка
    int line;       // Строка начала для отладки
    
    ConditionalState(bool active, bool was, int l) 
        : is_active(active), was_true(was), line(l) {}
};

// Класс препроцессора
class Preprocessor {
private:
    std::string source;
    std::string::const_iterator current;
    std::string::const_iterator start;
    
    int line_number;
    int column;
    
    // Карта макросов
    std::unordered_map<std::string, Macro> macros;
    
    // Стек условной компиляции
    std::vector<ConditionalState> conditional_stack;
    
    // Флаги состояния
    bool skipping; // Пропускаем ли текущий блок (условная компиляция)
    bool had_error; // Были ли ошибки
    
    // Выходной буфер
    std::string output;
    
    // Для отслеживания рекурсии макросов
    std::vector<std::string> macro_expansion_stack;
    static constexpr int MAX_EXPANSION_DEPTH = 100;
    
    // Вспомогательные методы
    char peek() const;
    char peek_next() const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;
    void skip_whitespace();
    void skip_line();
    
    // Чтение лексем
    std::string read_identifier();
    std::string read_line();
    
    // Обработка директив
    DirectiveType identify_directive(const std::string& dir);
    void handle_directive();
    void handle_define();
    void handle_undef();
    void handle_ifdef();
    void handle_ifndef();
    void handle_else();
    void handle_endif();
    
    // Проверка defined
    bool is_defined(const std::string& name);
    
    // Макроподстановка
    std::string expand_macros(const std::string& text);
    
    // Обработка комментариев
    void process_comments();
    std::string remove_comments(const std::string& input);
    
    // Добавление в выходной буфер с сохранением строк
    void append_output(const std::string& text);
    void append_output(char c);
    
    // Сообщения об ошибках
    void error(const std::string& message);
    
public:
    // Конструктор
    Preprocessor(const std::string& source_code);
    
    // Основной метод обработки
    std::string process();
    
    // Управление макросами
    void define_macro(const std::string& name, const std::string& value);
    void undefine_macro(const std::string& name);
    bool is_macro_defined(const std::string& name) const;
    
    // Получение информации об ошибках
    bool has_errors() const { return had_error; }
    std::string get_errors() const { return error_messages; }
    
private:
    std::string error_messages;
    std::string current_filename;
};

#endif // PREPROCESSOR_H