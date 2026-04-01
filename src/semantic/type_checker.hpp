#ifndef SEMANTIC_TYPE_CHECKER_HPP
#define SEMANTIC_TYPE_CHECKER_HPP

#include "type.hpp"
#include "../lexer/tokens.hpp"
#include <string>

namespace semantic {

// Проверка типов и правил совместимости
class TypeChecker {
public:
    static bool isCompatible(const Type* expected, const Type* actual) {
        if (!expected || !actual) return false;
        
        // Точное совпадение
        if (expected->equals(actual)) return true;
        
        // Неявное преобразование int в float
        if (expected->isFloat() && actual->isInt()) return true;
        
        // Неявное преобразование int/float в bool
        if (expected->isBool() && actual->isArithmetic()) return true;
        
        if (expected->isVoid() || actual->isVoid()) return false;
        
        if (expected->isError() || actual->isError()) return true;
        
        return false;
    }
    
    // Проверка точного соответствия типов
    static bool isExactMatch(const Type* expected, const Type* actual) {
        if (!expected || !actual) return false;
        return expected->equals(actual);
    }
    
    // Тип результата бинарной операции
    static Type* getBinaryResultType(TokenType op, Type* left, Type* right) {
        if (!left || !right) return getErrorType();
        
        if (left->isError() || right->isError()) {
            return getErrorType();
        }
        
        // Арифметические операторы (+, -, *, /)
        if (op == TokenType::tkn_ADDITION || 
            op == TokenType::tkn_SUBTRACTION ||
            op == TokenType::tkn_MULTIPLICATION ||
            op == TokenType::tkn_SEGMENTATION) {
            
            // Оба операнда должны быть арифметическими (int/float)
            if (!left->isArithmetic() || !right->isArithmetic()) {
                return getErrorType();
            }
            
            // Если один из операндов float, результат float
            if (left->isFloat() || right->isFloat()) {
                static Type floatType(TypeKind::FLOAT);
                return &floatType;
            }
            
            // Иначе int
            static Type intType(TypeKind::INT);
            return &intType;
        }
        
        // Оператор остатка от деления (%)
        if (op == TokenType::tkn_MODULO) {
            
            // Оба операнда int
            if (!left->isInteger() || !right->isInteger()) {
                return getErrorType();
            }
            
            // Результат всегда int
            static Type intType(TypeKind::INT);
            return &intType;
        }
        
        // Операторы сравнения
        if (op == TokenType::tkn_EQUAL ||
            op == TokenType::tkn_NOT_EQUAL ||
            op == TokenType::tkn_LESS ||
            op == TokenType::tkn_LESS_OR_EQUAL ||
            op == TokenType::tkn_MORE ||
            op == TokenType::tkn_MORE_OR_EQUAL) {
            
            if (!isCompatible(left, right) && !isCompatible(right, left)) {
                return getErrorType();
            }
            
            static Type boolType(TypeKind::BOOL);
            return &boolType;
        }
        
        // Логические операторы
        if (op == TokenType::tkn_AND || op == TokenType::tkn_OR) {
            if (!left->isBool() || !right->isBool()) {
                return getErrorType();
            }
            
            static Type boolType(TypeKind::BOOL);
            return &boolType;
        }
        
        // Простое присваивание (=)
        if (op == TokenType::tkn_ASSIGNMENT) {
            // Тип результата - тип левого операнда
            return left;
        }
        
        // Составные операторы присваивания (+=, -=, *=, /=)
        if (op == TokenType::tkn_ASSIGNMENT_AFTER_ADDITION ||
            op == TokenType::tkn_ASSIGNMENT_AFTER_SUBTRACTION ||
            op == TokenType::tkn_ASSIGNMENT_AFTER_MULTIPLICATION ||
            op == TokenType::tkn_ASSIGNMENT_AFTER_SEGMENTATION) {
            
            // Определяем базовый оператор для проверки
            TokenType baseOp;
            switch (op) {
                case TokenType::tkn_ASSIGNMENT_AFTER_ADDITION:
                    baseOp = TokenType::tkn_ADDITION;
                    break;
                case TokenType::tkn_ASSIGNMENT_AFTER_SUBTRACTION:
                    baseOp = TokenType::tkn_SUBTRACTION;
                    break;
                case TokenType::tkn_ASSIGNMENT_AFTER_MULTIPLICATION:
                    baseOp = TokenType::tkn_MULTIPLICATION;
                    break;
                case TokenType::tkn_ASSIGNMENT_AFTER_SEGMENTATION:
                    baseOp = TokenType::tkn_SEGMENTATION;
                    break;
                default:
                    return getErrorType();
            }
            
            // Проверяем, что операция допустима для типов
            Type* opResult = getBinaryResultType(baseOp, left, right);
            if (opResult->isError()) {
                return getErrorType();
            }
            
            // Результат - тип левого операнда
            return left;
        }
        
        // Составной оператор присваивания после деления по модулю (%=)
        if (op == TokenType::tkn_ASSIGNMENT_AFTER_MODULO) {

            // Проверяем, что оба операнда int
            if (!left->isInteger() || !right->isInteger()) {
                return getErrorType();
            }
            
            // Результат - тип левого операнда
            return left;
        }
        
        return getErrorType();
    }
    
    // Тип результата унарной операции
    static Type* getUnaryResultType(TokenType op, Type* operand) {
        if (!operand) return getErrorType();
        
        if (operand->isError()) return getErrorType();
        
        // Унарный минус (-)
        if (op == TokenType::tkn_SUBTRACTION) {
            if (!operand->isArithmetic()) {
                return getErrorType();
            }
            return operand;
        }
        
        // Логическое отрицание (!)
        if (op == TokenType::tkn_NOT) {
            if (!operand->isBool()) {
                return getErrorType();
            }
            static Type boolType(TypeKind::BOOL);
            return &boolType;
        }
        
        // Инкремент, декремент
        if (op == TokenType::tkn_INCREMENT || op == TokenType::tkn_DECREMENT) {
            if (!operand->isArithmetic()) {
                return getErrorType();
            }
            return operand;
        }
        
        return getErrorType();
    }
    
    // Проверка совместимости аргументов с параметрами функции
    static bool checkArgumentTypes(const std::vector<Type*>& paramTypes,
                                   const std::vector<Type*>& argTypes) {
        if (paramTypes.size() != argTypes.size()) {
            return false;
        }
        
        for (size_t i = 0; i < paramTypes.size(); ++i) {
            if (!isCompatible(paramTypes[i], argTypes[i])) {
                return false;
            }
        }
        
        return true;
    }
    
    // Проверка, можно ли присвоить значение переменной
    static bool isAssignable(const Type* varType, const Type* valueType) {
        if (!varType || !valueType) return false;
        
        if (valueType->isVoid()) return false;
                
        return isCompatible(varType, valueType);
    }
    
    // Проверка типа условия (if, while, for)
    static bool isValidCondition(const Type* conditionType) {
        if (!conditionType) return false;
        
        if (conditionType->isError()) return true;
        
        return conditionType->isBool() || conditionType->isArithmetic();
    }
    
    // Получение общего типа для двух типов
    static Type* getCommonType(Type* a, Type* b) {
        if (!a || !b) return getErrorType();
        
        if (a->isError() || b->isError()) return getErrorType();
        
        // Если оба int
        if (a->isInt() && b->isInt()) {
            static Type intType(TypeKind::INT);
            return &intType;
        }
        
        // Если один из них float
        if (a->isFloat() || b->isFloat()) {
            static Type floatType(TypeKind::FLOAT);
            return &floatType;
        }
        
        // Если оба bool
        if (a->isBool() && b->isBool()) {
            static Type boolType(TypeKind::BOOL);
            return &boolType;
        }
        
        return getErrorType();
    }
    
    static int getTypeSize(const Type* type) {
        if (!type) return 0;
        
        switch (type->getKind()) {
            case TypeKind::INT:
                return 4;
            case TypeKind::FLOAT:
                return 4;
            case TypeKind::BOOL:
                return 1;
            case TypeKind::VOID:
                return 0;
            case TypeKind::STRING:
                return 8;
            case TypeKind::STRUCT:
                return calculateStructSize(type);
            default:
                return 0;
        }
    }
    
    static int getTypeAlignment(const Type* type) {
        if (!type) return 1;
        
        switch (type->getKind()) {
            case TypeKind::INT:
            case TypeKind::FLOAT:
                return 4;
            case TypeKind::BOOL:
                return 1;
            case TypeKind::STRING:
                return 8;
            case TypeKind::STRUCT:
                return calculateStructAlignment(type);
            default:
                return 1;
        }
    }
    
    static int alignOffset(int offset, int alignment) {
        if (alignment <= 1) return offset;
        int remainder = offset % alignment;
        if (remainder == 0) return offset;
        return offset + (alignment - remainder);
    }
    
private:
    static int calculateStructSize(const Type* structType) {
        if (!structType->isStruct()) return 0;
        
        auto structInfo = structType->getStructInfo();
        if (!structInfo) return 0;
        
        int totalSize = 0;
        for (const auto& field : structInfo->fields) {
            totalSize += getTypeSize(field.second);
        }
        
        return totalSize;
    }
    
    static int calculateStructAlignment(const Type* structType) {
        if (!structType->isStruct()) return 1;
        
        auto structInfo = structType->getStructInfo();
        if (!structInfo) return 1;
        
        int maxAlignment = 1;
        for (const auto& field : structInfo->fields) {
            int fieldAlignment = getTypeAlignment(field.second);
            if (fieldAlignment > maxAlignment) {
                maxAlignment = fieldAlignment;
            }
        }
        return maxAlignment;
    }
};

}

#endif
