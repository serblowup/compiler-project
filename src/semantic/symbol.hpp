#ifndef SEMANTIC_SYMBOL_HPP
#define SEMANTIC_SYMBOL_HPP

#include "type.hpp"
#include <string>
#include <vector>
#include <memory>

namespace semantic {

/* Виды символов:
*  - переменная
*  - параметр функции
*  - функция
*  - структура
*  - поле структуры
*/
enum class SymbolKind {
    VARIABLE,
    PARAMETER,
    FUNCTION,
    STRUCT,
    FIELD
};

inline std::string symbolKindToString(SymbolKind kind) {
    switch (kind) {
        case SymbolKind::VARIABLE: return "variable";
        case SymbolKind::PARAMETER: return "parameter";
        case SymbolKind::FUNCTION: return "function";
        case SymbolKind::STRUCT: return "struct";
        case SymbolKind::FIELD: return "field";
        default: return "unknown";
    }
}

struct ParameterInfo {
    std::string name;
    Type* type;
    int line;
    int column;
    
    ParameterInfo(const std::string& n, Type* t, int l, int c)
        : name(n), type(t), line(l), column(c) {}
};

struct SymbolInfo {
    /* Для символа:
    *  - имя
    *  - тип
    *  - вид
    *  - строка, где объявлен
    *  - позиция в строке, где объявлен
    */
    std::string name;
    Type* type;
    SymbolKind kind;
    int line;
    int column;
    
    /* Для функций:
    *  - возвращаемый тип
    *  - параметры
    */
    Type* returnType;
    std::vector<ParameterInfo> parameters;
    
    /* Для структур:
    *  - поля
    */
    std::vector<SymbolInfo*> fields;
    
    /* Для переменных/параметров:
    *  - смещение в стеке (если не определено, то -1)
    *  - размер в байтах
    */
    int stackOffset;
    int size;
    
    /* Флаги:
    *  - инициализирована ли переменная?
       - это константа?
    */
    bool isInitialized;
    bool isConstant;
    
    SymbolInfo() 
        : type(nullptr), kind(SymbolKind::VARIABLE), line(0), column(0),
          returnType(nullptr), stackOffset(-1), size(0), 
          isInitialized(false), isConstant(false) {}
    
    SymbolInfo(const std::string& n, Type* t, SymbolKind k, int l, int c)
        : name(n), type(t), kind(k), line(l), column(c),
          returnType(nullptr), stackOffset(-1), size(0),
          isInitialized(false), isConstant(false) {}
    
    // Для переменных
    static SymbolInfo createVariable(const std::string& name, Type* type, int line, int column) {
        return SymbolInfo(name, type, SymbolKind::VARIABLE, line, column);
    }
    
    // Для параметров
    static SymbolInfo createParameter(const std::string& name, Type* type, int line, int column) {
        return SymbolInfo(name, type, SymbolKind::PARAMETER, line, column);
    }
    
    // Для функций
    static SymbolInfo createFunction(const std::string& name, Type* returnType, int line, int column) {
        SymbolInfo info(name, returnType, SymbolKind::FUNCTION, line, column);
        info.returnType = returnType;
        return info;
    }
    
    // Для структур
    static SymbolInfo createStruct(const std::string& name, int line, int column) {
        return SymbolInfo(name, nullptr, SymbolKind::STRUCT, line, column);
    }
    
    // Для полей
    static SymbolInfo createField(const std::string& name, Type* type, int line, int column) {
        return SymbolInfo(name, type, SymbolKind::FIELD, line, column);
    }
    
    void addParameter(const std::string& paramName, Type* paramType, int line, int column) {
        parameters.emplace_back(paramName, paramType, line, column);
    }
    
    void addField(SymbolInfo* field) {
        fields.push_back(field);
    }
    
    bool hasParameter(const std::string& paramName) const {
        for (const auto& p : parameters) {
            if (p.name == paramName) {
                return true;
            }
        }
        return false;
    }
    
    const ParameterInfo* getParameter(size_t index) const {
        if (index < parameters.size()) {
            return &parameters[index];
        }
        return nullptr;
    }
    
    size_t getParameterCount() const {
        return parameters.size();
    }
    
    // Строковое представление для вывода
    std::string toString() const {
        std::string result = name + " : " + (type ? type->toString() : "unknown");
        result += " [" + symbolKindToString(kind) + "]";
        
        if (kind == SymbolKind::FUNCTION) {
            result += " -> " + (returnType ? returnType->toString() : "void");
            if (!parameters.empty()) {
                result += " (params: ";
                for (size_t i = 0; i < parameters.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += parameters[i].name + ": " + parameters[i].type->toString();
                }
                result += ")";
            }
        }
        
        if (kind == SymbolKind::STRUCT && !fields.empty()) {
            result += " {";
            for (size_t i = 0; i < fields.size(); ++i) {
                if (i > 0) result += ", ";
                result += fields[i]->name + ": " + fields[i]->type->toString();
            }
            result += "}";
        }
        
        result += " at line " + std::to_string(line) + ":" + std::to_string(column);
        
        if (stackOffset >= 0) {
            result += " [offset: " + std::to_string(stackOffset) + ", size: " + std::to_string(size) + "]";
        }
        
        return result;
    }
    
    // Проверка, является ли символ функцией
    bool isFunction() const {
        return kind == SymbolKind::FUNCTION;
    }
    
    // Проверка, является ли символ структурой
    bool isStruct() const {
        return kind == SymbolKind::STRUCT;
    }
    
    // Проверка, является ли символ переменной
    bool isVariable() const {
        return kind == SymbolKind::VARIABLE || kind == SymbolKind::PARAMETER;
    }
    
    // Проверка, является ли символ полем
    bool isField() const {
        return kind == SymbolKind::FIELD;
    }
};

}

#endif