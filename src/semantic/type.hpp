#ifndef SEMANTIC_TYPE_HPP
#define SEMANTIC_TYPE_HPP

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace semantic {

enum class TypeKind {
    /* Типы:
    *  - int
    *  - float
    *  - bool
    *  - void
    *  - string
    *  - struct
    *  - function
    *  - error
    */
    INT,
    FLOAT,
    BOOL,
    VOID,
    STRING,
    STRUCT,
    FUNCTION,
    ERROR
};

inline std::string typeKindToString(TypeKind kind) {
    switch (kind) {
        case TypeKind::INT: return "int";
        case TypeKind::FLOAT: return "float";
        case TypeKind::BOOL: return "bool";
        case TypeKind::VOID: return "void";
        case TypeKind::STRING: return "string";
        case TypeKind::STRUCT: return "struct";
        case TypeKind::FUNCTION: return "function";
        case TypeKind::ERROR: return "error";
        default: return "unknown";
    }
}

struct Type;

// Структура для struct (имя + поля)
struct StructType {
    std::string name;
    std::unordered_map<std::string, Type*> fields;
    
    StructType(const std::string& n) : name(n) {}
    
    void addField(const std::string& fieldName, Type* fieldType) {
        fields[fieldName] = fieldType;
    }
    
    Type* getFieldType(const std::string& fieldName) const {
        auto it = fields.find(fieldName);
        return it != fields.end() ? it->second : nullptr;
    }
    
    bool hasField(const std::string& fieldName) const {
        return fields.find(fieldName) != fields.end();
    }
};

// Структура для function (параметры + возвращаемый тип)
struct FunctionType {
    std::vector<Type*> parameters;
    Type* returnType;
    
    FunctionType(Type* ret) : returnType(ret) {}
    
    void addParameter(Type* paramType) {
        parameters.push_back(paramType);
    }
    
    size_t getParameterCount() const {
        return parameters.size();
    }
    
    Type* getParameterType(size_t index) const {
        if (index < parameters.size()) {
            return parameters[index];
        }
        return nullptr;
    }
    
    bool matches(const std::vector<Type*>& argTypes) const {
        if (parameters.size() != argTypes.size()) {
            return false;
        }
        
        for (size_t i = 0; i < parameters.size(); ++i) {
            if (parameters[i] != argTypes[i]) {
                return false;
            }
        }
        
        return true;
    }
};

class Type {
private:
    TypeKind kind;
    
    std::shared_ptr<StructType> structInfo;
    
    std::shared_ptr<FunctionType> functionInfo;
    
public:
    Type(TypeKind kind) : kind(kind) {}
    
    Type(const std::string& structName) 
        : kind(TypeKind::STRUCT), structInfo(std::make_shared<StructType>(structName)) {}
    
    Type(Type* returnType) 
        : kind(TypeKind::FUNCTION), functionInfo(std::make_shared<FunctionType>(returnType)) {}
    
    TypeKind getKind() const { return kind; }
    
    bool isInt() const { return kind == TypeKind::INT; }
    bool isFloat() const { return kind == TypeKind::FLOAT; }
    bool isBool() const { return kind == TypeKind::BOOL; }
    bool isVoid() const { return kind == TypeKind::VOID; }
    bool isString() const { return kind == TypeKind::STRING; }
    bool isStruct() const { return kind == TypeKind::STRUCT; }
    bool isFunction() const { return kind == TypeKind::FUNCTION; }
    bool isError() const { return kind == TypeKind::ERROR; }
    
    bool isNumeric() const {
        return kind == TypeKind::INT || kind == TypeKind::FLOAT;
    }
    
    bool isArithmetic() const {
        return kind == TypeKind::INT || kind == TypeKind::FLOAT;
    }
    
    std::shared_ptr<StructType> getStructInfo() const {
        return structInfo;
    }
    
    const std::string& getStructName() const {
        static const std::string empty;
        return structInfo ? structInfo->name : empty;
    }
    
    void setStructInfo(std::shared_ptr<StructType> info) {
        structInfo = info;
    }
    
    std::shared_ptr<FunctionType> getFunctionInfo() const {
        return functionInfo;
    }
    
    Type* getReturnType() const {
        return functionInfo ? functionInfo->returnType : nullptr;
    }
    
    void addParameter(Type* paramType) {
        if (functionInfo) {
            functionInfo->addParameter(paramType);
        }
    }
    
    // Сравнение типов
    bool equals(const Type* other) const {
        if (!other) return false;
        
        if (kind != other->kind) return false;
        
        // Для структур нужно сравнить имена
        if (kind == TypeKind::STRUCT) {
            return getStructName() == other->getStructName();
        }
        
        // Для функций нужно сравнить сигнатуры
        if (kind == TypeKind::FUNCTION) {
            if (!functionInfo || !other->functionInfo) return false;
            
            if (functionInfo->returnType != other->functionInfo->returnType) {
                return false;
            }
            
            if (functionInfo->parameters.size() != other->functionInfo->parameters.size()) {
                return false;
            }
            
            for (size_t i = 0; i < functionInfo->parameters.size(); ++i) {
                if (functionInfo->parameters[i] != other->functionInfo->parameters[i]) {
                    return false;
                }
            }
            
            return true;
        }
        
        return true;
    }
    
    // Строковое представление для вывода
    std::string toString() const {
        if (kind == TypeKind::STRUCT) {
            return "struct " + getStructName();
        }
        if (kind == TypeKind::FUNCTION) {
            std::string result = "fn(";
            if (functionInfo) {
                for (size_t i = 0; i < functionInfo->parameters.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += functionInfo->parameters[i]->toString();
                }
            }
            result += ") -> ";
            result += getReturnType() ? getReturnType()->toString() : "void";
            return result;
        }
        return typeKindToString(kind);
    }
};

// Статический тип ошибки
inline Type* getErrorType() {
    static Type errorType(TypeKind::ERROR);
    return &errorType;
}

// Проверка на тип ошибки
inline bool isErrorType(const Type* type) {
    return type && type->isError();
}

}

#endif