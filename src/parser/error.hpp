#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

enum class RecoveryType {
    NONE,
    PANIC_MODE,
    INSERT_TOKEN,
    DELETE_TOKEN,
    REPLACE_TOKEN
};

// Ошибки
enum class ErrorCode {
    EXPECTED_EXPRESSION,
    EXPECTED_STATEMENT,
    EXPECTED_SEMICOLON,
    EXPECTED_RPAREN,
    EXPECTED_RBRACE,
    EXPECTED_LPAREN,
    EXPECTED_LBRACE,
    EXPECTED_IDENTIFIER,
    EXPECTED_TYPE,
    EXPECTED_PARAMETER,
    MISMATCHED_PARENTHESES,
    UNEXPECTED_TOKEN,
    INVALID_VARIABLE_NAME,
    MISSING_FUNCTION_NAME,
    MISSING_RETURN_TYPE,
    INVALID_EXPRESSION,
    EXPECTED_EXPRESSION_AFTER_RETURN
};

struct FormattedError {
    std::string filename;
    int line;
    int column;
    ErrorCode code;
    std::string message;
    std::string context_line;
    std::string pointer_line;
    std::string suggestion;
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "[ERROR] " << filename << ":" << line << ":" << column 
            << ": " << errorCodeToString(code) << ": " << message;
        if (!suggestion.empty()) {
            oss << "\n  Suggestion: " << suggestion;
        }
        oss << "\n";
        if (!context_line.empty()) {
            oss << "[CONTEXT] " << context_line << "\n";
            oss << "[POINTER] " << pointer_line << "\n";
        }
        return oss.str();
    }
    
    static std::string errorCodeToString(ErrorCode code) {
        switch (code) {
            case ErrorCode::EXPECTED_EXPRESSION: return "EXPECTED_EXPRESSION";
            case ErrorCode::EXPECTED_STATEMENT: return "EXPECTED_STATEMENT";
            case ErrorCode::EXPECTED_SEMICOLON: return "EXPECTED_SEMICOLON";
            case ErrorCode::EXPECTED_RPAREN: return "EXPECTED_RPAREN";
            case ErrorCode::EXPECTED_RBRACE: return "EXPECTED_RBRACE";
            case ErrorCode::EXPECTED_LPAREN: return "EXPECTED_LPAREN";
            case ErrorCode::EXPECTED_LBRACE: return "EXPECTED_LBRACE";
            case ErrorCode::EXPECTED_IDENTIFIER: return "EXPECTED_IDENTIFIER";
            case ErrorCode::EXPECTED_TYPE: return "EXPECTED_TYPE";
            case ErrorCode::EXPECTED_PARAMETER: return "EXPECTED_PARAMETER";
            case ErrorCode::MISMATCHED_PARENTHESES: return "MISMATCHED_PARENTHESES";
            case ErrorCode::UNEXPECTED_TOKEN: return "UNEXPECTED_TOKEN";
            case ErrorCode::INVALID_VARIABLE_NAME: return "INVALID_VARIABLE_NAME";
            case ErrorCode::MISSING_FUNCTION_NAME: return "MISSING_FUNCTION_NAME";
            case ErrorCode::MISSING_RETURN_TYPE: return "MISSING_RETURN_TYPE";
            case ErrorCode::INVALID_EXPRESSION: return "INVALID_EXPRESSION";
            case ErrorCode::EXPECTED_EXPRESSION_AFTER_RETURN: return "EXPECTED_EXPRESSION_AFTER_RETURN";
            default: return "UNKNOWN_ERROR";
        }
    }
};

struct ErrorMetrics {
    int total_errors_detected;
    int recovered_errors;
    int panic_mode_recoveries;
    int phrase_level_recoveries;
    int insert_token_recoveries;
    int delete_token_recoveries;
    int replace_token_recoveries;
    int max_error_count;
    bool limit_reached;
    std::vector<RecoveryType> recovery_history;
    
    ErrorMetrics() 
        : total_errors_detected(0), recovered_errors(0), 
          panic_mode_recoveries(0), phrase_level_recoveries(0),
          insert_token_recoveries(0), delete_token_recoveries(0),
          replace_token_recoveries(0), max_error_count(100), 
          limit_reached(false) {}
    
    void mark_error() {
        total_errors_detected++;
        if (max_error_count > 0 && total_errors_detected >= max_error_count) {
            limit_reached = true;
        }
    }
    
    void mark_recovered(RecoveryType type) {
        recovered_errors++;
        recovery_history.push_back(type);
        
        switch (type) {
            case RecoveryType::PANIC_MODE:
                panic_mode_recoveries++;
                phrase_level_recoveries++;
                break;
            case RecoveryType::INSERT_TOKEN:
                insert_token_recoveries++;
                phrase_level_recoveries++;
                break;
            case RecoveryType::DELETE_TOKEN:
                delete_token_recoveries++;
                phrase_level_recoveries++;
                break;
            case RecoveryType::REPLACE_TOKEN:
                replace_token_recoveries++;
                phrase_level_recoveries++;
                break;
            default:
                break;
        }
    }
    
    double get_recovery_rate() const {
        if (total_errors_detected == 0) return 100.0;
        return (static_cast<double>(recovered_errors) / total_errors_detected) * 100.0;
    }
    
    double get_phrase_level_rate() const {
        if (total_errors_detected == 0) return 100.0;
        return (static_cast<double>(phrase_level_recoveries) / total_errors_detected) * 100.0;
    }
    
    std::string toString() const {
        std::ostringstream oss;
        oss << "Метрики ошибок:\n";
        oss << "  Всего обнаружено ошибок: " << total_errors_detected << "\n";
        oss << "  Восстановлено ошибок: " << recovered_errors << "\n";
        oss << "  Процент восстановления: " << std::fixed << std::setprecision(2) << get_recovery_rate() << "%\n";
        oss << "  Восстановлений на уровне фраз: " << phrase_level_recoveries << "\n";
        oss << "    - Вставка токена: " << insert_token_recoveries << "\n";
        oss << "    - Удаление токена: " << delete_token_recoveries << "\n";
        oss << "    - Замена токена: " << replace_token_recoveries << "\n";
        oss << "  Восстановлений в режиме паники: " << panic_mode_recoveries << "\n";
        if (limit_reached) {
            oss << "  Лимит ошибок достигнут: да\n";
        } else {
            oss << "  Лимит ошибок достигнут: нет\n";
        }
        return oss.str();
    }
};

struct RecoverySuggestion {
    std::string message;
    std::string suggestion;
    TokenType expected_token;
    TokenType found_token;
    int line;
    int column;
    
    RecoverySuggestion(const std::string& msg, const std::string& sugg, 
                       TokenType expected, TokenType found, int l, int c)
        : message(msg), suggestion(sugg), expected_token(expected), 
          found_token(found), line(l), column(c) {}
};

#endif
