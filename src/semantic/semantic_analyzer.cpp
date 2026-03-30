#include "semantic_analyzer.hpp"
#include "../parser/ast.hpp"
#include <sstream>
#include <algorithm>

namespace semantic {

    SemanticAnalyzer::SemanticAnalyzer(const std::string& filename,
                                       const std::string& sourceCode)
    : symbolTable(std::make_unique<SymbolTable>()),
      errorCollector(std::make_unique<ErrorCollector>(100)),
      filename(filename),
      sourceCode(sourceCode),
      currentFunction(""),
      currentFunctionReturnType(nullptr),
      inLoop(false),
      inFunctionBody(false),
      errorType(getErrorType()) {

        errorCollector->setFilename(filename);
    }

    void SemanticAnalyzer::analyze(ProgramNode* program) {
        if (!program) return;

        program->accept(this);
    }

    const std::vector<SemanticError>& SemanticAnalyzer::getErrors() const {
        return errorCollector->getErrors();
    }

    bool SemanticAnalyzer::hasErrors() const {
        return errorCollector->hasErrors();
    }

    std::string SemanticAnalyzer::getAllErrors() const {
        return errorCollector->getAllErrors();
    }

    SymbolTable SemanticAnalyzer::getSymbolTableSnapshot() const {
        return symbolTable->getSnapshot();
    }

    void SemanticAnalyzer::clear() {
        symbolTable->clear();
        errorCollector->clear();
        currentFunction = "";
        currentFunctionReturnType = nullptr;
        inLoop = false;
        inFunctionBody = false;
        memoryLayouts.clear();
    }

    std::string SemanticAnalyzer::getContextLine(int line) {
        if (sourceCode.empty()) return "";

        int currentLine = 1;
        size_t pos = 0;
        size_t start = 0;

        while (pos < sourceCode.size() && currentLine < line) {
            if (sourceCode[pos] == '\n') {
                currentLine++;
                start = pos + 1;
            }
            pos++;
        }

        if (currentLine != line) return "";

        size_t end = sourceCode.find('\n', start);
        if (end == std::string::npos) end = sourceCode.size();

        return sourceCode.substr(start, end - start);
    }

    std::string SemanticAnalyzer::getPointer(int column) {
        if (column < 1) column = 1;
        return std::string(column - 1, ' ') + "^";
    }

    void SemanticAnalyzer::reportError(int line, int column,
                                       SemanticErrorCode code,
                                       const std::string& message) {
        SemanticError error(filename, line, column, code, message);

        std::string contextLine = getContextLine(line);
        if (!contextLine.empty()) {
            error.setContext(contextLine, column);
        }

        error.generateSuggestion();
        errorCollector->addError(error);
    }

    void SemanticAnalyzer::reportTypeMismatch(int line, int column,
                                              Type* expected, Type* actual,
                                              const std::string& context) {
        SemanticError error(filename, line, column, expected, actual, context);

        std::string contextLine = getContextLine(line);
        if (!contextLine.empty()) {
            error.setContext(contextLine, column);
        }

        error.generateSuggestion();
        errorCollector->addError(error);
    }

    void SemanticAnalyzer::reportUndeclared(int line, int column, const std::string& name) {
        SemanticError error(filename, line, column,
                           SemanticErrorCode::UNDECLARED_IDENTIFIER,
                           "идентификатор '" + name + "' не объявлен");
        error.identifierName = name;

        std::string contextLine = getContextLine(line);
        if (!contextLine.empty()) {
            error.setContext(contextLine, column);
        }

        error.generateSuggestion();
        errorCollector->addError(error);
    }

    void SemanticAnalyzer::reportDuplicate(int line, int column, const std::string& name) {
        SemanticError error(filename, line, column,
                           SemanticErrorCode::DUPLICATE_DECLARATION,
                           "идентификатор '" + name + "' уже объявлен в этой области видимости");
        error.identifierName = name;

        std::string contextLine = getContextLine(line);
        if (!contextLine.empty()) {
            error.setContext(contextLine, column);
        }

        error.generateSuggestion();
        errorCollector->addError(error);
    }

    bool SemanticAnalyzer::checkNotDeclared(const std::string& name, int line, int column) {
        if (symbolTable->existsCurrent(name)) {
            reportDuplicate(line, column, name);
            return false;
        }
        return true;
    }

    void SemanticAnalyzer::annotateNode(ExpressionNode* node, Type* type) {
        node->setType(type);
    }

    Type* SemanticAnalyzer::resolveIdentifier(IdentifierExprNode* node) {
        SymbolInfo* symbol = symbolTable->lookup(node->getName());

        if (!symbol) {
            reportUndeclared(node->getLine(), node->getColumn(), node->getName());
            return errorType;
        }

        node->setSymbol(symbol);
        node->setType(symbol->type);

        return symbol->type;
    }

    Type* SemanticAnalyzer::checkBinaryOp(BinaryExprNode* node) {
        node->getLeft()->accept(this);
        node->getRight()->accept(this);

        Type* leftType = node->getLeft()->getType();
        Type* rightType = node->getRight()->getType();

        if (isErrorType(leftType) || isErrorType(rightType)) {
            annotateNode(node, errorType);
            return errorType;
        }

        if (!leftType || !rightType) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               nullptr, nullptr, "бинарной операции");
            annotateNode(node, errorType);
            return errorType;
        }

        Type* resultType = TypeChecker::getBinaryResultType(node->getOperator(), leftType, rightType);

        if (resultType->isError()) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               nullptr, nullptr, "бинарной операции");
            annotateNode(node, errorType);
            return errorType;
        }

        annotateNode(node, resultType);
        return resultType;
    }

    Type* SemanticAnalyzer::checkUnaryOp(UnaryExprNode* node) {
        node->getOperand()->accept(this);

        Type* operandType = node->getOperand()->getType();

        if (isErrorType(operandType)) {
            annotateNode(node, errorType);
            return errorType;
        }

        if (!operandType) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               nullptr, nullptr, "унарной операции");
            annotateNode(node, errorType);
            return errorType;
        }

        Type* resultType = TypeChecker::getUnaryResultType(node->getOperator(), operandType);

        if (resultType->isError()) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               nullptr, nullptr, "унарной операции");
            annotateNode(node, errorType);
            return errorType;
        }

        annotateNode(node, resultType);
        return resultType;
    }

    Type* SemanticAnalyzer::checkAssignment(AssignmentExprNode* node) {
        node->getTarget()->accept(this);
        node->getValue()->accept(this);

        Type* targetType = node->getTarget()->getType();
        Type* valueType = node->getValue()->getType();

        if (isErrorType(targetType) || isErrorType(valueType)) {
            annotateNode(node, errorType);
            return errorType;
        }

        if (!targetType || !valueType) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               nullptr, nullptr, "присваивании");
            annotateNode(node, errorType);
            return errorType;
        }

        if (!TypeChecker::isAssignable(targetType, valueType)) {
            reportTypeMismatch(node->getLine(), node->getColumn(),
                               targetType, valueType, "присваивании");
            annotateNode(node, errorType);
            return errorType;
        }

        if (node->getOperator() != TokenType::tkn_ASSIGNMENT) {
            Type* opResultType = TypeChecker::getBinaryResultType(
                node->getOperator(), targetType, valueType);

            if (opResultType->isError()) {
                reportTypeMismatch(node->getLine(), node->getColumn(),
                                   targetType, valueType,
                                   "составном присваивании");
                annotateNode(node, errorType);
                return errorType;
            }
        }

        annotateNode(node, targetType);
        return targetType;
    }

    Type* SemanticAnalyzer::checkFunctionCall(CallExprNode* node) {
        node->getCallee()->accept(this);

        Type* calleeType = node->getCallee()->getType();

        if (isErrorType(calleeType)) {
            annotateNode(node, errorType);
            return errorType;
        }

        if (!calleeType) {
            reportError(node->getLine(), node->getColumn(),
                        SemanticErrorCode::INVALID_OPERATION,
                        "не удалось определить тип вызываемого выражения");
            annotateNode(node, errorType);
            return errorType;
        }

        if (!calleeType->isFunction()) {
            std::string funcName = "выражение";
            if (auto* idExpr = dynamic_cast<IdentifierExprNode*>(node->getCallee())) {
                funcName = idExpr->getName();
            }
            reportError(node->getLine(), node->getColumn(),
                        SemanticErrorCode::INVALID_OPERATION,
                        "'" + funcName + "' не является функцией");
            annotateNode(node, errorType);
            return errorType;
        }

        auto funcInfo = calleeType->getFunctionInfo();
        if (!funcInfo) {
            reportError(node->getLine(), node->getColumn(),
                        SemanticErrorCode::INVALID_OPERATION,
                        "не удалось получить информацию о функции");
            annotateNode(node, errorType);
            return errorType;
        }

        size_t expectedCount = funcInfo->getParameterCount();
        size_t actualCount = node->getArguments().size();

        if (expectedCount != actualCount) {
            std::string funcName = "функции";
            if (auto* idExpr = dynamic_cast<IdentifierExprNode*>(node->getCallee())) {
                funcName = "'" + idExpr->getName() + "'";
            }

            reportError(node->getLine(), node->getColumn(),
                        SemanticErrorCode::ARGUMENT_COUNT_MISMATCH,
                        "несоответствие количества аргументов при вызове " + funcName +
                        ": ожидалось " + std::to_string(expectedCount) +
                        ", получено " + std::to_string(actualCount));
            annotateNode(node, errorType);
            return errorType;
        }

        std::vector<Type*> argTypes;
        for (const auto& arg : node->getArguments()) {
            arg->accept(this);
            Type* argType = arg->getType();
            argTypes.push_back(argType);

            if (isErrorType(argType)) {
                annotateNode(node, errorType);
                return errorType;
            }
        }

        for (size_t i = 0; i < expectedCount; ++i) {
            Type* paramType = funcInfo->getParameterType(i);
            Type* argType = argTypes[i];

            if (!paramType || !argType) {
                reportError(node->getArguments()[i]->getLine(),
                            node->getArguments()[i]->getColumn(),
                            SemanticErrorCode::ARGUMENT_TYPE_MISMATCH,
                            "не удалось определить тип аргумента " + std::to_string(i + 1));
                annotateNode(node, errorType);
                return errorType;
            }

            if (!TypeChecker::isCompatible(paramType, argType)) {
                reportTypeMismatch(node->getArguments()[i]->getLine(),
                                   node->getArguments()[i]->getColumn(),
                                   paramType, argType,
                                   "аргументе " + std::to_string(i + 1));
                annotateNode(node, errorType);
                return errorType;
            }
        }

        Type* returnType = funcInfo->returnType;
        annotateNode(node, returnType);

        return returnType;
    }

    void SemanticAnalyzer::visitProgramNode(ProgramNode* node) {
        for (const auto& decl : node->getDeclarations()) {
            decl->accept(this);
        }
    }

    void SemanticAnalyzer::visitFunctionDeclNode(FunctionDeclNode* node) {
        std::string funcName = node->getName();

        if (!checkNotDeclared(funcName, node->getLine(), node->getColumn())) {
            return;
        }

        Type* returnType = nullptr;
        if (node->getReturnType() == "int") {
            static Type intType(TypeKind::INT);
            returnType = &intType;
        } else if (node->getReturnType() == "float") {
            static Type floatType(TypeKind::FLOAT);
            returnType = &floatType;
        } else if (node->getReturnType() == "bool") {
            static Type boolType(TypeKind::BOOL);
            returnType = &boolType;
        } else if (node->getReturnType() == "void") {
            static Type voidType(TypeKind::VOID);
            returnType = &voidType;
        } else if (node->getReturnType().find("struct ") == 0) {
            std::string structName = node->getReturnType().substr(7);
            SymbolInfo* structSymbol = symbolTable->lookup(structName);
            if (!structSymbol || !structSymbol->isStruct()) {
                reportError(node->getLine(), node->getColumn(),
                            SemanticErrorCode::UNDECLARED_IDENTIFIER,
                            "структура '" + structName + "' не объявлена");
                return;
            }
            static Type structType(structName);
            returnType = &structType;
        } else {
            SymbolInfo* structSymbol = symbolTable->lookup(node->getReturnType());
            if (!structSymbol || !structSymbol->isStruct()) {
                reportError(node->getLine(), node->getColumn(),
                            SemanticErrorCode::UNDECLARED_IDENTIFIER,
                            "тип '" + node->getReturnType() + "' не объявлен");
                return;
            }
            static Type structType(node->getReturnType());
            returnType = &structType;
        }

        Type* functionType = new Type(returnType);

        SymbolInfo funcSymbol = SymbolInfo::createFunction(funcName, returnType,
                                                           node->getLine(), node->getColumn());

        for (const auto& param : node->getParameters()) {
            Type* paramType = nullptr;
            if (param->getType() == "int") {
                static Type intType(TypeKind::INT);
                paramType = &intType;
            } else if (param->getType() == "float") {
                static Type floatType(TypeKind::FLOAT);
                paramType = &floatType;
            } else if (param->getType() == "bool") {
                static Type boolType(TypeKind::BOOL);
                paramType = &boolType;
            } else {
                SymbolInfo* structSymbol = symbolTable->lookup(param->getType());
                if (!structSymbol || !structSymbol->isStruct()) {
                    reportError(param->getLine(), param->getColumn(),
                                SemanticErrorCode::UNDECLARED_IDENTIFIER,
                                "тип '" + param->getType() + "' не объявлен");
                    continue;
                }
                static Type structType(param->getType());
                paramType = &structType;
            }

            functionType->addParameter(paramType);
            funcSymbol.addParameter(param->getName(), paramType,
                                    param->getLine(), param->getColumn());
        }

        funcSymbol.type = functionType;

        symbolTable->insert(funcName, funcSymbol);
        
        symbolTable->enterScope(ScopeType::FUNCTION, funcName);

        std::string prevFunction = currentFunction;
        Type* prevReturnType = currentFunctionReturnType;
        currentFunction = funcName;
        currentFunctionReturnType = returnType;

        MemoryLayoutInfo layout;
        layout.functionName = funcName;
        int currentOffset = 0;
        std::vector<std::tuple<std::string, std::string, int, int>> variables;

        for (const auto& param : node->getParameters()) {
            Type* paramType = nullptr;
            if (param->getType() == "int") {
                static Type intType(TypeKind::INT);
                paramType = &intType;
            } else if (param->getType() == "float") {
                static Type floatType(TypeKind::FLOAT);
                paramType = &floatType;
            } else if (param->getType() == "bool") {
                static Type boolType(TypeKind::BOOL);
                paramType = &boolType;
            } else {
                static Type structType(param->getType());
                paramType = &structType;
            }

            SymbolInfo paramSymbol = SymbolInfo::createParameter(
                param->getName(), paramType, param->getLine(), param->getColumn());
            paramSymbol.isInitialized = true;

            int alignment = TypeChecker::getTypeAlignment(paramType);
            currentOffset = TypeChecker::alignOffset(currentOffset, alignment);
            paramSymbol.stackOffset = currentOffset;
            paramSymbol.size = TypeChecker::getTypeSize(paramType);
            
            variables.emplace_back(param->getName(), paramType->toString(), currentOffset, paramSymbol.size);
            currentOffset += paramSymbol.size;

            symbolTable->insert(param->getName(), paramSymbol);
        }

        inFunctionBody = true;

        if (node->getBody()) {
            node->getBody()->accept(this);
        }

        inFunctionBody = false;

        layout.totalStackSize = (currentOffset + 15) & ~15;
        layout.variables = variables;
        memoryLayouts.push_back(layout);

        symbolTable->exitScope();

        currentFunction = prevFunction;
        currentFunctionReturnType = prevReturnType;
    }

    void SemanticAnalyzer::calculateMemoryLayout(FunctionDeclNode* func, int& currentOffset, 
                                                  std::vector<std::tuple<std::string, std::string, int, int>>& variables) {
        (void)func;
        (void)currentOffset;
        (void)variables;
    }

    void SemanticAnalyzer::visitStructDeclNode(StructDeclNode* node) {
        std::string structName = node->getName();

        if (!checkNotDeclared(structName, node->getLine(), node->getColumn())) {
            return;
        }

        SymbolInfo structSymbol = SymbolInfo::createStruct(structName,
                                                           node->getLine(), node->getColumn());

        Type* structType = new Type(structName);
        auto structInfo = std::make_shared<StructType>(structName);
        structType->setStructInfo(structInfo);
        structSymbol.type = structType;

        symbolTable->enterScope(ScopeType::STRUCT, structName);

        for (const auto& field : node->getFields()) {
            Type* fieldType = nullptr;
            if (field->getType() == "int") {
                static Type intType(TypeKind::INT);
                fieldType = &intType;
            } else if (field->getType() == "float") {
                static Type floatType(TypeKind::FLOAT);
                fieldType = &floatType;
            } else if (field->getType() == "bool") {
                static Type boolType(TypeKind::BOOL);
                fieldType = &boolType;
            } else if (field->getType() == "string") {
                static Type stringType(TypeKind::STRING);
                fieldType = &stringType;
            } else {
                SymbolInfo* fieldStructSymbol = symbolTable->lookup(field->getType());
                if (!fieldStructSymbol || !fieldStructSymbol->isStruct()) {
                    reportError(field->getLine(), field->getColumn(),
                                SemanticErrorCode::UNDECLARED_IDENTIFIER,
                                "тип '" + field->getType() + "' не объявлен");
                    continue;
                }
                static Type fieldStructType(field->getType());
                fieldType = &fieldStructType;
            }

            if (!checkNotDeclared(field->getName(), field->getLine(), field->getColumn())) {
                continue;
            }

            SymbolInfo fieldSymbol = SymbolInfo::createField(
                field->getName(), fieldType, field->getLine(), field->getColumn());

            symbolTable->insert(field->getName(), fieldSymbol);
            structSymbol.addField(&fieldSymbol);

            structInfo->addField(field->getName(), fieldType);
        }

        symbolTable->exitScope();

        symbolTable->insert(structName, structSymbol);
    }

    void SemanticAnalyzer::visitParamNode(ParamNode* node) {
        (void)node;
    }

    void SemanticAnalyzer::visitBlockStmtNode(BlockStmtNode* node) {
        symbolTable->enterScope(ScopeType::BLOCK);
        
        for (const auto& stmt : node->getStatements()) {
            stmt->accept(this);
            if (errorCollector->isLimitReached()) break;
        }
        
        symbolTable->exitScope();
    }

    void SemanticAnalyzer::visitVarDeclStmtNode(VarDeclStmtNode* node) {
        std::string varName = node->getName();

        if (!checkNotDeclared(varName, node->getLine(), node->getColumn())) {
            return;
        }

        Type* varType = nullptr;
        if (node->getType() == "int") {
            static Type intType(TypeKind::INT);
            varType = &intType;
        } else if (node->getType() == "float") {
            static Type floatType(TypeKind::FLOAT);
            varType = &floatType;
        } else if (node->getType() == "bool") {
            static Type boolType(TypeKind::BOOL);
            varType = &boolType;
        } else if (node->getType() == "string") {
            static Type stringType(TypeKind::STRING);
            varType = &stringType;
        } else if (node->getType() == "void") {
            reportError(node->getLine(), node->getColumn(),
                        SemanticErrorCode::VOID_VARIABLE,
                        "нельзя объявить переменную типа void");
            return;
        } else {
            SymbolInfo* structSymbol = symbolTable->lookup(node->getType());
            if (!structSymbol || !structSymbol->isStruct()) {
                reportError(node->getLine(), node->getColumn(),
                            SemanticErrorCode::UNDECLARED_IDENTIFIER,
                            "тип '" + node->getType() + "' не объявлен");
                return;
            }
            static Type structType(node->getType());
            varType = &structType;
        }

        SymbolInfo varSymbol = SymbolInfo::createVariable(varName, varType,
                                                          node->getLine(), node->getColumn());

        if (node->hasInitializer()) {
            node->getInitializer()->accept(this);

            Type* initType = node->getInitializer()->getType();

            if (!isErrorType(initType) && initType) {
                if (!TypeChecker::isAssignable(varType, initType)) {
                    reportTypeMismatch(node->getInitializer()->getLine(),
                                       node->getInitializer()->getColumn(),
                                       varType, initType,
                                       "инициализации переменной '" + varName + "'");
                    varSymbol.isInitialized = false;
                } else {
                    varSymbol.isInitialized = true;
                }
            } else {
                varSymbol.isInitialized = false;
            }
        }

        symbolTable->insert(varName, varSymbol);
    }

    void SemanticAnalyzer::visitIfStmtNode(IfStmtNode* node) {
        node->getCondition()->accept(this);

        Type* condType = node->getCondition()->getType();

        if (!isErrorType(condType) && condType) {
            if (!TypeChecker::isValidCondition(condType)) {
                reportTypeMismatch(node->getCondition()->getLine(),
                                   node->getCondition()->getColumn(),
                                   nullptr, condType,
                                   "условии оператора if");
            }
        }

        symbolTable->enterScope(ScopeType::IF_THEN);
        node->getThenBranch()->accept(this);
        symbolTable->exitScope();

        if (node->hasElse()) {
            symbolTable->enterScope(ScopeType::IF_ELSE);
            node->getElseBranch()->accept(this);
            symbolTable->exitScope();
        }
    }

    void SemanticAnalyzer::visitWhileStmtNode(WhileStmtNode* node) {
        node->getCondition()->accept(this);

        Type* condType = node->getCondition()->getType();

        if (!isErrorType(condType) && condType) {
            if (!TypeChecker::isValidCondition(condType)) {
                reportTypeMismatch(node->getCondition()->getLine(),
                                   node->getCondition()->getColumn(),
                                   nullptr, condType,
                                   "условии оператора while");
            }
        }

        bool prevInLoop = inLoop;
        inLoop = true;

        symbolTable->enterScope(ScopeType::WHILE_BODY);
        node->getBody()->accept(this);
        symbolTable->exitScope();

        inLoop = prevInLoop;
    }

    void SemanticAnalyzer::visitForStmtNode(ForStmtNode* node) {
        symbolTable->enterScope(ScopeType::FOR_LOOP);
        
        if (node->hasInit()) {
            node->getInit()->accept(this);
        }

        if (node->hasCondition()) {
            node->getCondition()->accept(this);

            Type* condType = node->getCondition()->getType();

            if (!isErrorType(condType) && condType) {
                if (!TypeChecker::isValidCondition(condType)) {
                    reportTypeMismatch(node->getCondition()->getLine(),
                                       node->getCondition()->getColumn(),
                                       nullptr, condType,
                                       "условии оператора for");
                }
            }
        }

        if (node->hasUpdate()) {
            node->getUpdate()->accept(this);
        }

        bool prevInLoop = inLoop;
        inLoop = true;

        symbolTable->enterScope(ScopeType::FOR_BODY);
        node->getBody()->accept(this);
        symbolTable->exitScope();

        inLoop = prevInLoop;
        
        symbolTable->exitScope();
    }

    void SemanticAnalyzer::visitReturnStmtNode(ReturnStmtNode* node) {
        if (node->hasValue()) {
            node->getValue()->accept(this);

            Type* valueType = node->getValue()->getType();

            if (currentFunction.empty()) {
                reportError(node->getLine(), node->getColumn(),
                            SemanticErrorCode::INVALID_RETURN_TYPE,
                            "оператор return вне функции");
                return;
            }

            if (currentFunctionReturnType && currentFunctionReturnType->isVoid()) {
                SemanticError error(filename, node->getLine(), node->getColumn(),
                                    SemanticErrorCode::INVALID_RETURN_TYPE,
                                    "функция " + currentFunction + " имеет тип void, но возвращает значение");
                error.expectedType = currentFunctionReturnType;
                error.actualType = valueType;

                std::string contextLine = getContextLine(node->getLine());
                if (!contextLine.empty()) {
                    error.setContext(contextLine, node->getColumn());
                }
                error.generateSuggestion();
                errorCollector->addError(error);
                return;
            }

            if (!isErrorType(valueType) && valueType && currentFunctionReturnType) {
                if (!TypeChecker::isCompatible(currentFunctionReturnType, valueType)) {
                    reportTypeMismatch(node->getValue()->getLine(),
                                       node->getValue()->getColumn(),
                                       currentFunctionReturnType, valueType,
                                       "возвращаемом значении функции " + currentFunction);
                }
            }

        } else {
            if (!currentFunction.empty() && currentFunctionReturnType &&
                !currentFunctionReturnType->isVoid()) {
                reportError(node->getLine(), node->getColumn(),
                            SemanticErrorCode::INVALID_RETURN_TYPE,
                            "функция " + currentFunction + " должна возвращать значение типа " +
                            currentFunctionReturnType->toString());
            }
        }
    }

    void SemanticAnalyzer::visitExprStmtNode(ExprStmtNode* node) {
        if (node->getExpression()) {
            node->getExpression()->accept(this);
        }
    }

    void SemanticAnalyzer::visitBinaryExprNode(BinaryExprNode* node) {
        checkBinaryOp(node);
    }

    void SemanticAnalyzer::visitUnaryExprNode(UnaryExprNode* node) {
        checkUnaryOp(node);
    }

    void SemanticAnalyzer::visitLiteralExprNode(LiteralExprNode* node) {
        Type* type = nullptr;

        if (auto intVal = node->getValue<int>()) {
            static Type intType(TypeKind::INT);
            type = &intType;
            (void)intVal;
        } else if (auto floatVal = node->getValue<double>()) {
            static Type floatType(TypeKind::FLOAT);
            type = &floatType;
            (void)floatVal;
        } else if (auto strVal = node->getValue<std::string>()) {
            static Type stringType(TypeKind::STRING);
            type = &stringType;
            (void)strVal;
        } else if (auto boolVal = node->getValue<bool>()) {
            static Type boolType(TypeKind::BOOL);
            type = &boolType;
            (void)boolVal;
        }

        annotateNode(node, type);
    }

    void SemanticAnalyzer::visitIdentifierExprNode(IdentifierExprNode* node) {
        resolveIdentifier(node);
    }

    void SemanticAnalyzer::visitCallExprNode(CallExprNode* node) {
        checkFunctionCall(node);
    }

    void SemanticAnalyzer::visitAssignmentExprNode(AssignmentExprNode* node) {
        checkAssignment(node);
    }

    std::string SemanticAnalyzer::getDecoratedASTString() const {
        std::ostringstream oss;
        oss << "Декорированное AST\n\n";
        oss << "(используйте --show-types для просмотра аннотаций типов в выводе AST)\n";
        return oss.str();
    }

    std::string SemanticAnalyzer::getValidationReport() const {
        std::ostringstream oss;
        
        oss << "\n";
        oss << "Отчёт о валидации\n";
        oss << "\n";
        
        oss << "Сводка по ошибкам:\n";
        oss << "   └── Всего ошибок: " << errorCollector->getErrorCount() << "\n";
        if (errorCollector->isLimitReached()) {
            oss << "   └── Внимание: достигнут лимит ошибок!\n";
        }
        oss << "\n";
        
        oss << "Символы по областям видимости:\n";
        std::string symStr = symbolTable->toString();
        std::istringstream iss(symStr);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "   " << line << "\n";
        }
        oss << "\n";
        
        oss << "Иерархия типов:\n";
        oss << getTypeHierarchy();
        oss << "\n";
        
        oss << "Статистика типов:\n";
        auto stats = getTypeStatistics();
        oss << "   ├── Всего выражений: " << stats.totalExpressions << "\n";
        oss << "   ├── Выражений с типами: " << stats.typedExpressions << "\n";
        oss << "   ├── Ошибочных типов: " << stats.errorTypes << "\n";
        oss << "   └── Распределение типов:\n";
        for (const auto& [typeName, count] : stats.typeDistribution) {
            oss << "       ├── " << typeName << ": " << count << "\n";
        }
        oss << "\n";
        
        oss << "Размещение в памяти:\n";
        auto layouts = getMemoryLayout();
        if (layouts.empty()) {
            oss << "   └── Нет информации о размещении в памяти\n";
        } else {
            for (const auto& layout : layouts) {
                oss << "   └── Функция: " << layout.functionName << "\n";
                oss << "       ├── Общий размер стека: " << layout.totalStackSize << " байт\n";
                if (!layout.variables.empty()) {
                    oss << "       └── Переменные:\n";
                    for (const auto& [name, type, offset, size] : layout.variables) {
                        oss << "           ├── " << name << ": " << type 
                            << " [смещение=" << offset << ", размер=" << size << "]\n";
                    }
                } else {
                    oss << "       └── (нет локальных переменных)\n";
                }
            }
        }
        oss << "\n";
        
        oss << "Конец отчёта!\n";
        
        return oss.str();
    }

    SemanticAnalyzer::TypeStatistics SemanticAnalyzer::getTypeStatistics() const {
        TypeStatistics stats;
        stats.totalExpressions = 0;
        stats.typedExpressions = 0;
        stats.errorTypes = 0;
        
        stats.typeDistribution["int"] = 0;
        stats.typeDistribution["float"] = 0;
        stats.typeDistribution["bool"] = 0;
        stats.typeDistribution["string"] = 0;
        stats.typeDistribution["void"] = 0;
        stats.typeDistribution["struct"] = 0;
        stats.typeDistribution["function"] = 0;
        stats.typeDistribution["error"] = 0;
        
        return stats;
    }

    std::string SemanticAnalyzer::getTypeHierarchy() const {
        std::ostringstream oss;
        
        oss << "   ├── Примитивные типы:\n";
        oss << "   │   ├── int (4 байта, выравнивание 4)\n";
        oss << "   │   ├── float (4 байта, выравнивание 4)\n";
        oss << "   │   ├── bool (1 байт, выравнивание 1)\n";
        oss << "   │   ├── void (0 байт)\n";
        oss << "   │   └── string (8 байт, выравнивание 8)\n";
        oss << "   │\n";
        oss << "   ├── Типы структур:\n";
        
        auto globalSymbols = symbolTable->getGlobalSymbols();
        bool hasStructs = false;
        for (const auto& sym : globalSymbols) {
            if (sym.isStruct() && sym.type) {
                hasStructs = true;
                int structSize = TypeChecker::getTypeSize(sym.type);
                oss << "   │   └── struct " << sym.name << " (" << structSize << " байт)\n";
                
                if (sym.type->getStructInfo()) {
                    for (const auto& field : sym.type->getStructInfo()->fields) {
                        int fieldSize = TypeChecker::getTypeSize(field.second);
                        oss << "   │       └── " << field.first << ": " 
                            << field.second->toString() << " (" << fieldSize << " байт)\n";
                    }
                }
            }
        }
        if (!hasStructs) {
            oss << "   │   └── (нет объявленных структур)\n";
        }
        oss << "   │\n";
        oss << "   └── Типы функций:\n";
        
        bool hasFunctions = false;
        for (const auto& sym : globalSymbols) {
            if (sym.isFunction()) {
                hasFunctions = true;
                oss << "       └── " << sym.name << "(";
                for (size_t i = 0; i < sym.parameters.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << sym.parameters[i].type->toString();
                }
                oss << ") -> " << (sym.returnType ? sym.returnType->toString() : "void") << "\n";
            }
        }
        if (!hasFunctions) {
            oss << "       └── (нет объявленных функций)\n";
        }
        
        return oss.str();
    }

    std::vector<SemanticAnalyzer::MemoryLayoutInfo> SemanticAnalyzer::getMemoryLayout() const {
        return memoryLayouts;
    }

}
