#ifndef SEMANTIC_ERROR_HPP
#define SEMANTIC_ERROR_HPP

#include "type.hpp"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace semantic {

/* Типы семантических ошибок:
*  - необъявленный идентификатор
*  - повторное объявление
*  - несоответствие типов
*  - несоответствие количества аргументов
*  - несоответствие типов аргументов
*  - неверный возвращаемый тип
*  - неверный тип условия
*  - использование до объявления
*  - неверная цель присваивания
*  - неинициализированная переменная
*  - отсутствует оператор return
*  - неверный тип операнда
*  - недопустимая операция
*  - переопределение функции
*  - переопределение структуры
*  - повторяющееся поле структуры
*  - переменная типа void
*  - присваивание константе
*  - несовместимые типы
*/
enum class SemanticErrorCode {
    UNDECLARED_IDENTIFIER,
    DUPLICATE_DECLARATION,
    TYPE_MISMATCH,
    ARGUMENT_COUNT_MISMATCH,
    ARGUMENT_TYPE_MISMATCH,
    INVALID_RETURN_TYPE,
    INVALID_CONDITION_TYPE,
    USE_BEFORE_DECLARATION,
    INVALID_ASSIGNMENT_TARGET,
    UNINITIALIZED_VARIABLE,
    MISSING_RETURN_STATEMENT,
    INVALID_OPERAND_TYPE,
    INVALID_OPERATION,
    FUNCTION_REDECLARATION,
    STRUCT_REDECLARATION,
    DUPLICATE_FIELD,
    VOID_VARIABLE,
    CONST_ASSIGNMENT,
    INCOMPATIBLE_TYPES
};

inline std::string errorCodeToString(SemanticErrorCode code) {
    switch (code) {
        case SemanticErrorCode::UNDECLARED_IDENTIFIER:
            return "необъявленный идентификатор";
        case SemanticErrorCode::DUPLICATE_DECLARATION:
            return "повторное объявление";
        case SemanticErrorCode::TYPE_MISMATCH:
            return "несоответствие типов";
        case SemanticErrorCode::ARGUMENT_COUNT_MISMATCH:
            return "несоответствие количества аргументов";
        case SemanticErrorCode::ARGUMENT_TYPE_MISMATCH:
            return "несоответствие типов аргументов";
        case SemanticErrorCode::INVALID_RETURN_TYPE:
            return "неверный возвращаемый тип";
        case SemanticErrorCode::INVALID_CONDITION_TYPE:
            return "неверный тип условия";
        case SemanticErrorCode::USE_BEFORE_DECLARATION:
            return "использование до объявления";
        case SemanticErrorCode::INVALID_ASSIGNMENT_TARGET:
            return "неверная цель присваивания";
        case SemanticErrorCode::UNINITIALIZED_VARIABLE:
            return "неинициализированная переменная";
        case SemanticErrorCode::MISSING_RETURN_STATEMENT:
            return "отсутствует оператор return";
        case SemanticErrorCode::INVALID_OPERAND_TYPE:
            return "неверный тип операнда";
        case SemanticErrorCode::INVALID_OPERATION:
            return "недопустимая операция";
        case SemanticErrorCode::FUNCTION_REDECLARATION:
            return "переопределение функции";
        case SemanticErrorCode::STRUCT_REDECLARATION:
            return "переопределение структуры";
        case SemanticErrorCode::DUPLICATE_FIELD:
            return "повторяющееся поле структуры";
        case SemanticErrorCode::VOID_VARIABLE:
            return "переменная типа void";
        case SemanticErrorCode::CONST_ASSIGNMENT:
            return "присваивание константе";
        case SemanticErrorCode::INCOMPATIBLE_TYPES:
            return "несовместимые типы";
        default:
            return "семантическая ошибка";
    }
}

struct SemanticError {
    std::string filename;
    int line;
    int column;
    SemanticErrorCode code;
    std::string message;
    std::string context_line;
    std::string pointer_line;
    
    Type* expectedType;
    Type* actualType;
    
    int expectedCount;
    int actualCount;
    
    std::string identifierName;
    std::string suggestion;
    
    SemanticError(const std::string& file, int l, int c, 
                  SemanticErrorCode cd, const std::string& msg)
        : filename(file), line(l), column(c), code(cd), message(msg),
          context_line(""), pointer_line(""),
          expectedType(nullptr), actualType(nullptr),
          expectedCount(0), actualCount(0),
          identifierName(""), suggestion("") {}
    
    SemanticError(const std::string& file, int l, int c,
                  Type* expected, Type* actual, const std::string& context = "")
        : filename(file), line(l), column(c),
          code(SemanticErrorCode::TYPE_MISMATCH),
          context_line(""), pointer_line(""),
          expectedType(expected), actualType(actual),
          expectedCount(0), actualCount(0),
          identifierName(""), suggestion("") {
        
        std::ostringstream oss;
        oss << "несоответствие типов";
        if (!context.empty()) {
            oss << " в " << context;
        }
        message = oss.str();
    }
    
    SemanticError(const std::string& file, int l, int c,
                  int expected, int actual, const std::string& funcName)
        : filename(file), line(l), column(c),
          code(SemanticErrorCode::ARGUMENT_COUNT_MISMATCH),
          context_line(""), pointer_line(""),
          expectedType(nullptr), actualType(nullptr),
          expectedCount(expected), actualCount(actual),
          identifierName(""), suggestion("") {
        
        std::ostringstream oss;
        oss << "несоответствие количества аргументов при вызове функции '" 
            << funcName << "': ожидалось " << expected 
            << ", получено " << actual;
        message = oss.str();
    }
    
    std::string toString() const {
        std::ostringstream oss;
        
        oss << "semantic error: " << errorCodeToString(code);
        if (!message.empty() && message != errorCodeToString(code)) {
            oss << " - " << message;
        }
        
        oss << "\n  --> " << filename << ":" << line << ":" << column << "\n";
        
        if (!context_line.empty()) {
            oss << line << "  " << context_line << "\n";
            
            std::string lineNumStr = std::to_string(line);
            int lineNumWidth = static_cast<int>(lineNumStr.length());
            int offset = lineNumWidth + 2 + (column - 1);
            oss << std::string(offset, ' ') << "^\n";
        }
        
        if (expectedType || actualType) {
            if (expectedType && !expectedType->isError()) {
                oss << "   = ожидалось: " << expectedType->toString() << "\n";
            }
            if (actualType && !actualType->isError()) {
                oss << "   = получено: " << actualType->toString() << "\n";
            }
        }
        
        if (expectedCount > 0 || actualCount > 0) {
            oss << "   = ожидалось аргументов: " << expectedCount << "\n";
            oss << "   = получено аргументов: " << actualCount << "\n";
        }
        
        if (!identifierName.empty()) {
            oss << "   = идентификатор: '" << identifierName << "'\n";
        }
        
        if (!suggestion.empty()) {
            oss << "   = подсказка: " << suggestion << "\n";
        }
        
        return oss.str();
    }
    
    void setContext(const std::string& lineContent, int errorColumn) {
        context_line = lineContent;
        pointer_line = std::string(errorColumn - 1, ' ') + "^";
    }
    
    void generateSuggestion() {
        switch (code) {
            case SemanticErrorCode::UNDECLARED_IDENTIFIER:
                suggestion = "проверьте, объявлена ли переменная '" + identifierName + 
                            "' в текущей области видимости";
                break;
            case SemanticErrorCode::DUPLICATE_DECLARATION:
                suggestion = "переименуйте идентификатор или удалите предыдущее объявление";
                break;
            case SemanticErrorCode::TYPE_MISMATCH:
                if (expectedType && actualType) {
                    if (actualType->isInt() && expectedType->isFloat()) {
                        suggestion = "используйте неявное преобразование: int автоматически преобразуется в float";
                    } else if (actualType->isFloat() && expectedType->isInt()) {
                        suggestion = "используйте явное преобразование: (int)значение";
                    } else if (actualType->isInt() && expectedType->isBool()) {
                        suggestion = "используйте неявное преобразование: 0 = false, ненулевое = true";
                    } else if (actualType->isFloat() && expectedType->isBool()) {
                        suggestion = "используйте неявное преобразование: 0.0 = false, ненулевое = true";
                    } else if (actualType->isArithmetic() && expectedType->isArithmetic()) {
                        suggestion = "проверьте типы операндов: для % требуются целые числа (int)";
                    } else {
                        suggestion = "приведите значение к ожидаемому типу";
                    }
                } else {
                    suggestion = "проверьте типы операндов";
                }
                break;
            case SemanticErrorCode::ARGUMENT_COUNT_MISMATCH:
                suggestion = "проверьте сигнатуру функции и количество передаваемых аргументов";
                break;
            case SemanticErrorCode::ARGUMENT_TYPE_MISMATCH:
                suggestion = "проверьте типы передаваемых аргументов";
                break;
            case SemanticErrorCode::INVALID_RETURN_TYPE:
                suggestion = "измените тип возвращаемого значения или тип функции";
                break;
            case SemanticErrorCode::INVALID_CONDITION_TYPE:
                suggestion = "условие должно иметь логический тип (bool) или числовой тип (int/float)";
                break;
            case SemanticErrorCode::UNINITIALIZED_VARIABLE:
                suggestion = "инициализируйте переменную перед использованием";
                break;
            case SemanticErrorCode::MISSING_RETURN_STATEMENT:
                suggestion = "добавьте оператор return с соответствующим значением";
                break;
            case SemanticErrorCode::INVALID_OPERATION:
                if (message.find("%") != std::string::npos) {
                    suggestion = "оператор % работает только с целыми числами (int)";
                } else if (message.find("%=") != std::string::npos) {
                    suggestion = "оператор %= работает только с целыми числами (int)";
                } else if (message.find("+=") != std::string::npos || 
                           message.find("-=") != std::string::npos ||
                           message.find("*=") != std::string::npos ||
                           message.find("/=") != std::string::npos) {
                    suggestion = "составные операторы требуют арифметических типов (int/float)";
                } else {
                    suggestion = "проверьте типы операндов";
                }
                break;
            case SemanticErrorCode::VOID_VARIABLE:
                suggestion = "нельзя объявить переменную типа void";
                break;
            case SemanticErrorCode::INVALID_ASSIGNMENT_TARGET:
                suggestion = "левая часть присваивания должна быть изменяемой (переменная или поле)";
                break;
            default:
                suggestion = "";
                break;
        }
    }
};

class ErrorCollector {
private:
    std::vector<SemanticError> errors;
    int maxErrors;
    bool limitReached;
    std::string currentFilename;
    
public:
    ErrorCollector(int max = 100) 
        : maxErrors(max), limitReached(false), currentFilename("<stdin>") {}
    
    void setFilename(const std::string& filename) {
        currentFilename = filename;
    }
    
    void addError(const SemanticError& error) {
        if (limitReached) return;
        
        errors.push_back(error);
        
        if (static_cast<int>(errors.size()) >= maxErrors) {
            limitReached = true;
            errors.emplace_back(currentFilename, 0, 0, 
                               SemanticErrorCode::INCOMPATIBLE_TYPES,
                               "достигнут лимит ошибок (" + std::to_string(maxErrors) + "), дальнейший анализ остановлен");
        }
    }
    
    void addError(const std::string& file, int line, int column,
                  SemanticErrorCode code, const std::string& message) {
        if (limitReached) return;
        addError(SemanticError(file, line, column, code, message));
    }
    
    void addTypeMismatch(int line, int column, Type* expected, Type* actual, 
                         const std::string& context = "") {
        if (limitReached) return;
        SemanticError err(currentFilename, line, column, expected, actual, context);
        addError(err);
    }
    
    void addArgumentCountMismatch(int line, int column, int expected, int actual,
                                  const std::string& funcName) {
        if (limitReached) return;
        SemanticError err(currentFilename, line, column, expected, actual, funcName);
        addError(err);
    }
    
    bool hasErrors() const {
        return !errors.empty();
    }
    
    bool isLimitReached() const {
        return limitReached;
    }
    
    const std::vector<SemanticError>& getErrors() const {
        return errors;
    }
    
    void clear() {
        errors.clear();
        limitReached = false;
    }
    
    std::string getAllErrors() const {
        std::ostringstream oss;
        for (const auto& error : errors) {
            oss << error.toString() << "\n";
        }
        return oss.str();
    }
    
    int getErrorCount() const {
        return errors.size();
    }
};

}

#endif