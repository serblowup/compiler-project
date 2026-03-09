#ifndef ERROR_HPP
#define ERROR_HPP

#include <string>
#include <vector>
#include <sstream>

enum class RecoveryType {
    NONE,
    PANIC_MODE,
    INSERT_TOKEN,
    DELETE_TOKEN,
    REPLACE_TOKEN
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
        oss << "  Процент восстановления: " << get_recovery_rate() << "%\n";
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