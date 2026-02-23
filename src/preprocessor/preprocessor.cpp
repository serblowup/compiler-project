#include "preprocessor.hpp"
#include <cctype>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>

// Конструктор
Preprocessor::Preprocessor(const std::string& source_code) 
    : source(source_code), 
      current(source.begin()),
      start(source.begin()),
      line_number(1),
      column(1),
      skipping(false),
      had_error(false) {
}

// Вспомогательные методы для работы с итераторами
char Preprocessor::peek() const {
    return is_at_end() ? '\0' : *current;
}

char Preprocessor::peek_next() const {
    if (current + 1 >= source.end()) return '\0';
    return *(current + 1);
}

char Preprocessor::advance() {
    if (is_at_end()) return '\0';
    char c = *current++;
    if (c == '\n') {
        line_number++;
        column = 1;
    } else {
        column++;
    }
    return c;
}

bool Preprocessor::match(char expected) {
    if (is_at_end()) return false;
    if (*current != expected) return false;
    advance();
    return true;
}

bool Preprocessor::is_at_end() const {
    return current == source.end();
}

void Preprocessor::skip_whitespace() {
    while (!is_at_end()) {
        char c = peek();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

void Preprocessor::skip_line() {
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
    if (!is_at_end()) {
        advance(); // пропускаем '\n'
    }
}

// Чтение лексем
std::string Preprocessor::read_identifier() {
    start = current;
    while (!is_at_end() && (std::isalnum(peek()) || peek() == '_')) {
        advance();
    }
    return std::string(start, current);
}

std::string Preprocessor::read_line() {
    start = current;
    while (!is_at_end() && peek() != '\n') {
        advance();
    }
    return std::string(start, current);
}

// Идентификация директив
DirectiveType Preprocessor::identify_directive(const std::string& dir) {
    static const std::unordered_map<std::string, DirectiveType> directive_map = {
        {"define", DirectiveType::DIRECTIVE_DEFINE},
        {"undef", DirectiveType::DIRECTIVE_UNDEF},
        {"ifdef", DirectiveType::DIRECTIVE_IFDEF},
        {"ifndef", DirectiveType::DIRECTIVE_IFNDEF},
        {"else", DirectiveType::DIRECTIVE_ELSE},
        {"endif", DirectiveType::DIRECTIVE_ENDIF}
    };
    
    auto it = directive_map.find(dir);
    return it != directive_map.end() ? it->second : DirectiveType::DIRECTIVE_NONE;
}

// Обработка директив
void Preprocessor::handle_directive() {
    skip_whitespace();
    std::string directive = read_identifier();
    
    DirectiveType type = identify_directive(directive);
    
    switch (type) {
        case DirectiveType::DIRECTIVE_DEFINE:
            handle_define();
            break;
        case DirectiveType::DIRECTIVE_UNDEF:
            handle_undef();
            break;
        case DirectiveType::DIRECTIVE_IFDEF:
            handle_ifdef();
            break;
        case DirectiveType::DIRECTIVE_IFNDEF:
            handle_ifndef();
            break;
        case DirectiveType::DIRECTIVE_ELSE:
            handle_else();
            break;
        case DirectiveType::DIRECTIVE_ENDIF:
            handle_endif();
            break;
        default:
            // Неизвестная директива - просто пропускаем
            skip_line();
            break;
    }
}

// #define
void Preprocessor::handle_define() {
    skip_whitespace();
    
    // Читаем имя макроса
    std::string name = read_identifier();
    if (name.empty()) {
        error("Ожидается имя макроса после #define");
        skip_line();
        return;
    }
    
    // Читаем тело макроса (все до конца строки)
    std::string replacement;
    while (!is_at_end() && peek() != '\n') {
        replacement += advance();
    }
    if (!is_at_end()) {
        advance(); // пропускаем '\n'
    }
    
    // Удаляем пробелы в начале и конце
    size_t start_pos = replacement.find_first_not_of(" \t");
    size_t end_pos = replacement.find_last_not_of(" \t");
    if (start_pos != std::string::npos && end_pos != std::string::npos) {
        replacement = replacement.substr(start_pos, end_pos - start_pos + 1);
    } else {
        replacement.clear();
    }
    
    // Если мы не в режиме пропуска, определяем макрос
    if (!skipping) {
        Macro macro(name, replacement, line_number);
        macros[name] = macro;
    }
}

// #undef
void Preprocessor::handle_undef() {
    skip_whitespace();
    std::string name = read_identifier();
    skip_line();
    
    if (!skipping) {
        macros.erase(name);
    }
}

// #ifdef
void Preprocessor::handle_ifdef() {
    skip_whitespace();
    std::string name = read_identifier();
    skip_line();
    
    bool defined = (macros.find(name) != macros.end());
    
    if (skipping) {
        conditional_stack.push_back(ConditionalState(false, false, line_number));
    } else {
        bool is_active = defined;
        conditional_stack.push_back(ConditionalState(is_active, is_active, line_number));
        skipping = !is_active;
    }
}

// #ifndef
void Preprocessor::handle_ifndef() {
    skip_whitespace();
    std::string name = read_identifier();
    skip_line();
    
    bool defined = (macros.find(name) != macros.end());
    
    if (skipping) {
        conditional_stack.push_back(ConditionalState(false, false, line_number));
    } else {
        bool is_active = !defined;
        conditional_stack.push_back(ConditionalState(is_active, is_active, line_number));
        skipping = !is_active;
    }
}

// #else
void Preprocessor::handle_else() {
    skip_line();
    
    if (conditional_stack.empty()) {
        error("#else без #if");
        return;
    }
    
    ConditionalState& state = conditional_stack.back();
    
    if (state.was_true) {
        skipping = true;
    } else {
        state.is_active = true;
        state.was_true = true;
        skipping = false;
    }
}

// #endif
void Preprocessor::handle_endif() {
    skip_line();
    
    if (conditional_stack.empty()) {
        error("#endif без #if");
        return;
    }
    
    conditional_stack.pop_back();
    skipping = !conditional_stack.empty() && !conditional_stack.back().is_active;
}

bool Preprocessor::is_defined(const std::string& name) {
    return macros.find(name) != macros.end();
}

// Макроподстановка
std::string Preprocessor::expand_macros(const std::string& text) {
    std::string result;
    size_t pos = 0;
    size_t text_len = text.length();
    
    while (pos < text_len) {
        // Проверяем на идентификатор
        if (std::isalpha(text[pos]) || text[pos] == '_') {
            size_t start = pos;
            while (pos < text_len && (std::isalnum(text[pos]) || text[pos] == '_')) {
                pos++;
            }
            std::string name = text.substr(start, pos - start);
            
            // Проверяем, есть ли такой макрос
            auto it = macros.find(name);
            if (it != macros.end()) {
                // Проверка на рекурсию
                if (std::find(macro_expansion_stack.begin(), macro_expansion_stack.end(), name) 
                    != macro_expansion_stack.end()) {
                    error("Рекурсивный макрос: " + name);
                    result += name;
                } else {
                    macro_expansion_stack.push_back(name);
                    if (macro_expansion_stack.size() > MAX_EXPANSION_DEPTH) {
                        error("Превышена глубина раскрытия макросов");
                        macro_expansion_stack.pop_back();
                        result += name;
                    } else {
                        std::string expanded = expand_macros(it->second.replacement);
                        macro_expansion_stack.pop_back();
                        result += expanded;
                    }
                }
            } else {
                result += name;
            }
        } else if (text[pos] == '"' || text[pos] == '\'') {
            // Пропускаем строки и символы
            char quote = text[pos++];
            result += quote;
            while (pos < text_len && text[pos] != quote) {
                if (text[pos] == '\\') {
                    result += text[pos++];
                }
                if (pos < text_len) {
                    result += text[pos++];
                }
            }
            if (pos < text_len) {
                result += text[pos++];
            }
        } else {
            result += text[pos++];
        }
    }
    
    return result;
}

void Preprocessor::process_comments() {
    source = remove_comments(source);
}

// Удаление комментариев
std::string Preprocessor::remove_comments(const std::string& input) {
    std::string result;
    size_t i = 0;
    bool in_string = false;
    bool in_char = false;
    bool in_line_comment = false;
    bool in_block_comment = false;
    
    while (i < input.length()) {
        if (!in_string && !in_char && !in_line_comment && !in_block_comment) {
            if (i + 1 < input.length()) {
                if (input[i] == '/' && input[i + 1] == '/') {
                    in_line_comment = true;
                    i += 2;
                    continue;
                } else if (input[i] == '/' && input[i + 1] == '*') {
                    in_block_comment = true;
                    i += 2;
                    continue;
                }
            }
        }
        
        if (in_line_comment) {
            if (input[i] == '\n') {
                in_line_comment = false;
                result += '\n';
            }
            i++;
            continue;
        }
        
        if (in_block_comment) {
            if (i + 1 < input.length() && input[i] == '*' && input[i + 1] == '/') {
                in_block_comment = false;
                i += 2;
                result += ' ';
                continue;
            }
            if (input[i] == '\n') {
                result += '\n';
            }
            i++;
            continue;
        }
        
        if (input[i] == '"' && !in_char && !in_line_comment && !in_block_comment) {
            in_string = !in_string;
        } else if (input[i] == '\'' && !in_string && !in_line_comment && !in_block_comment) {
            in_char = !in_char;
        }
        
        result += input[i];
        i++;
    }
    
    if (in_block_comment) {
        error("Незавершенный многострочный комментарий");
    }
    
    return result;
}

// Добавление в выходной буфер
void Preprocessor::append_output(const std::string& text) {
    output += text;
}

void Preprocessor::append_output(char c) {
    output += c;
}

// Сообщения об ошибках
void Preprocessor::error(const std::string& message) {
    had_error = true;
    std::stringstream ss;
    ss << (current_filename.empty() ? "<stdin>" : current_filename)
       << ":" << line_number << ": ошибка: " << message << "\n";
    error_messages += ss.str();
}

void Preprocessor::define_macro(const std::string& name, const std::string& value) {
    Macro macro(name, value, 0);
    macros[name] = macro;
}

void Preprocessor::undefine_macro(const std::string& name) {
    macros.erase(name);
}

bool Preprocessor::is_macro_defined(const std::string& name) const {
    return macros.find(name) != macros.end();
}

// Основной метод обработки
std::string Preprocessor::process() {
    // Сначала удаляем комментарии
    process_comments();
    
    // Сбрасываем итераторы после удаления комментариев
    current = source.begin();
    line_number = 1;
    column = 1;
    skipping = false;
    output.clear();
    macro_expansion_stack.clear();
    conditional_stack.clear();
    
    while (!is_at_end()) {
        // Проверяем начало директивы препроцессора
        if (peek() == '#') {
            advance();
            
            // Проверяем, не является ли это частью строки
            if (peek() != '#' && !skipping) {
                handle_directive();
                continue;
            }
        }
        
        // Если не директива или мы в режиме пропуска, просто копируем
        if (!skipping) {
            // Копируем строку до конца
            std::string line;
            while (!is_at_end() && peek() != '\n') {
                line += advance();
            }
            
            // Применяем макроподстановку
            std::string expanded = expand_macros(line);
            append_output(expanded);
            
            // Добавляем символ новой строки
            if (!is_at_end() && peek() == '\n') {
                append_output(advance());
            }
        } else {
            skip_line();
        }
    }
    
    return output;
}