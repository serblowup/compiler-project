#include "ir_generator.hpp"
#include <iostream>

namespace ir {

IRGenerator::IRGenerator(semantic::SymbolTable* sym_table)
    : program(nullptr)
    , current_function(nullptr)
    , current_block(nullptr)
    , symbol_table(sym_table)
    , temp_counter(0)
    , label_counter(0) {}

std::string IRGenerator::newTemp() {
    return "t" + std::to_string(++temp_counter);
}

std::string IRGenerator::newLabel() {
    return "L" + std::to_string(++label_counter);
}

void IRGenerator::emit(std::unique_ptr<Instruction> instr) {
    // Если это инструкция LABEL, создаём новый блок
    if (instr->kind == InstrKind::LABEL) {
        std::string block_name = instr->dest.name;
        current_block = current_function->createBlock(block_name);
        return;
    }
    
    if (current_block) {
        current_block->addInstruction(std::move(instr));
    }
}

void IRGenerator::emit(InstrKind kind, const Operand& dest, const Operand& src1) {
    auto instr = std::make_unique<Instruction>(kind);
    instr->dest = dest;
    instr->src1 = src1;
    emit(std::move(instr));
}

void IRGenerator::emit(InstrKind kind, const Operand& dest, const Operand& src1, const Operand& src2) {
    auto instr = std::make_unique<Instruction>(kind);
    instr->dest = dest;
    instr->src1 = src1;
    instr->src2 = src2;
    emit(std::move(instr));
}

Operand IRGenerator::loadVariable(const std::string& name, semantic::Type* type) {
    Operand temp = Operand::Temp(newTemp());
    auto instr = std::make_unique<Instruction>(InstrKind::LOAD);
    instr->dest = temp;
    instr->src1 = Operand::Var(name);
    instr->type = type;
    emit(std::move(instr));
    return temp;
}

void IRGenerator::storeVariable(const std::string& name, const Operand& value) {
    auto instr = std::make_unique<Instruction>(InstrKind::STORE);
    instr->dest = Operand::Var(name);
    instr->src1 = value;
    emit(std::move(instr));
}

semantic::Type* IRGenerator::getTypeFromAST(ExpressionNode* node) {
    if (node && node->getType()) {
        return node->getType();
    }
    static semantic::Type error_type(semantic::TypeKind::ERROR);
    return &error_type;
}

Operand IRGenerator::generateExpression(ExpressionNode* expr) {
    if (!expr) {
        return Operand::ConstInt(0);
    }
    
    expr->accept(this);
    
    if (expr_stack.empty()) {
        return Operand::ConstInt(0);
    }
    
    Operand result = expr_stack.top();
    expr_stack.pop();
    return result;
}

// Генерация выражений
void IRGenerator::visitLiteralExprNode(LiteralExprNode* node) {
    if (auto val = node->getValue<int>()) {
        expr_stack.push(Operand::ConstInt(*val));
    } else if (auto val = node->getValue<double>()) {
        expr_stack.push(Operand::ConstFloat(static_cast<float>(*val)));
    } else if (auto val = node->getValue<bool>()) {
        expr_stack.push(Operand::ConstBool(*val));
    } else {
        expr_stack.push(Operand::ConstInt(0));
    }
}

void IRGenerator::visitIdentifierExprNode(IdentifierExprNode* node) {
    semantic::Type* type = node->getType();
    Operand loaded = loadVariable(node->getName(), type);
    expr_stack.push(loaded);
}

void IRGenerator::visitBinaryExprNode(BinaryExprNode* node) {
    TokenType op = node->getOperator();
    
    Operand left = generateExpression(node->getLeft());
    Operand right = generateExpression(node->getRight());
    
    InstrKind kind;
    switch (op) {
        case TokenType::tkn_ADDITION: kind = InstrKind::ADD; break;
        case TokenType::tkn_SUBTRACTION: kind = InstrKind::SUB; break;
        case TokenType::tkn_MULTIPLICATION: kind = InstrKind::MUL; break;
        case TokenType::tkn_SEGMENTATION: kind = InstrKind::DIV; break;
        case TokenType::tkn_MODULO: kind = InstrKind::MOD; break;
        case TokenType::tkn_EQUAL: kind = InstrKind::CMP_EQ; break;
        case TokenType::tkn_NOT_EQUAL: kind = InstrKind::CMP_NE; break;
        case TokenType::tkn_LESS: kind = InstrKind::CMP_LT; break;
        case TokenType::tkn_LESS_OR_EQUAL: kind = InstrKind::CMP_LE; break;
        case TokenType::tkn_MORE: kind = InstrKind::CMP_GT; break;
        case TokenType::tkn_MORE_OR_EQUAL: kind = InstrKind::CMP_GE; break;
        case TokenType::tkn_AND: kind = InstrKind::AND; break;
        case TokenType::tkn_OR: kind = InstrKind::OR; break;
        default: kind = InstrKind::MOVE; break;
    }
    
    Operand result = Operand::Temp(newTemp());
    emit(kind, result, left, right);
    expr_stack.push(result);
}

void IRGenerator::visitUnaryExprNode(UnaryExprNode* node) {
    Operand operand = generateExpression(node->getOperand());
    TokenType op = node->getOperator();
    
    InstrKind kind;
    switch (op) {
        case TokenType::tkn_SUBTRACTION: kind = InstrKind::NEG; break;
        case TokenType::tkn_NOT: kind = InstrKind::NOT; break;
        default: kind = InstrKind::MOVE; break;
    }
    
    if (kind == InstrKind::NEG || kind == InstrKind::NOT) {
        Operand result = Operand::Temp(newTemp());
        emit(kind, result, operand);
        expr_stack.push(result);
    } else {
        expr_stack.push(operand);
    }
}

void IRGenerator::visitAssignmentExprNode(AssignmentExprNode* node) {
    Operand value = generateExpression(node->getValue());
    
    if (auto* id = dynamic_cast<IdentifierExprNode*>(node->getTarget())) {
        storeVariable(id->getName(), value);
    }
    
    expr_stack.push(value);
}

void IRGenerator::visitCallExprNode(CallExprNode* node) {
    std::vector<Operand> args;
    for (const auto& arg : node->getArguments()) {
        args.push_back(generateExpression(arg.get()));
    }
    
    std::string func_name;
    if (auto* id = dynamic_cast<IdentifierExprNode*>(node->getCallee())) {
        func_name = id->getName();
    } else {
        func_name = "unknown";
    }
    
    auto instr = std::make_unique<Instruction>(InstrKind::CALL);
    Operand result = Operand::Temp(newTemp());
    instr->dest = result;
    instr->src1 = Operand::Var(func_name);
    instr->args = args;
    
    if (node->getType()) {
        instr->type = node->getType();
    }
    
    emit(std::move(instr));
    expr_stack.push(result);
}

// Генерация операторов
void IRGenerator::visitBlockStmtNode(BlockStmtNode* node) {
    for (const auto& stmt : node->getStatements()) {
        stmt->accept(this);
    }
}

void IRGenerator::visitIfStmtNode(IfStmtNode* node) {
    std::string then_label = newLabel();
    std::string else_label = newLabel();
    std::string end_label = newLabel();
    
    BasicBlock* current = current_block;
    
    Operand cond = generateExpression(node->getCondition());
    
    auto jump_if_not = std::make_unique<Instruction>(InstrKind::JUMP_IF_NOT);
    jump_if_not->src1 = cond;
    jump_if_not->target_label = else_label;
    emit(std::move(jump_if_not));
    
    auto jump_to_then = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_then->target_label = then_label;
    emit(std::move(jump_to_then));
    
    BasicBlock* else_block = nullptr;
    BasicBlock* then_block = nullptr;
    
    auto label_then = std::make_unique<Instruction>(InstrKind::LABEL);
    label_then->dest = Operand::Label(then_label);
    emit(std::move(label_then));
    then_block = current_block;
    
    node->getThenBranch()->accept(this);
    
    auto jump_to_end = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_end->target_label = end_label;
    emit(std::move(jump_to_end));
    
    auto label_else = std::make_unique<Instruction>(InstrKind::LABEL);
    label_else->dest = Operand::Label(else_label);
    emit(std::move(label_else));
    else_block = current_block;
    
    if (node->hasElse()) {
        node->getElseBranch()->accept(this);
    }
    
    auto label_end = std::make_unique<Instruction>(InstrKind::LABEL);
    label_end->dest = Operand::Label(end_label);
    emit(std::move(label_end));
    
    BasicBlock* test_block = current;
    if (test_block) {
        test_block->addSuccessor(then_block);
        test_block->addSuccessor(else_block);
    }
    if (then_block) {
        then_block->addSuccessor(current_block);
    }
    if (else_block) {
        else_block->addSuccessor(current_block);
    }
}

void IRGenerator::visitWhileStmtNode(WhileStmtNode* node) {
    std::string test_label = newLabel();
    std::string body_label = newLabel();
    std::string end_label = newLabel();
    
    BasicBlock* entry_block = current_block;
    
    auto jump_to_test = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_test->target_label = test_label;
    emit(std::move(jump_to_test));
    
    auto label_test = std::make_unique<Instruction>(InstrKind::LABEL);
    label_test->dest = Operand::Label(test_label);
    emit(std::move(label_test));
    BasicBlock* test_block = current_block;
    
    Operand cond = generateExpression(node->getCondition());
    
    auto jump_if_not = std::make_unique<Instruction>(InstrKind::JUMP_IF_NOT);
    jump_if_not->src1 = cond;
    jump_if_not->target_label = end_label;
    emit(std::move(jump_if_not));
    
    auto jump_to_body = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_body->target_label = body_label;
    emit(std::move(jump_to_body));
    
    auto label_body = std::make_unique<Instruction>(InstrKind::LABEL);
    label_body->dest = Operand::Label(body_label);
    emit(std::move(label_body));
    BasicBlock* body_block = current_block;
    
    node->getBody()->accept(this);
    
    auto jump_back = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_back->target_label = test_label;
    emit(std::move(jump_back));
    
    auto label_end = std::make_unique<Instruction>(InstrKind::LABEL);
    label_end->dest = Operand::Label(end_label);
    emit(std::move(label_end));
    BasicBlock* end_block = current_block;
    
    if (entry_block) {
        entry_block->addSuccessor(test_block);
    }
    if (test_block) {
        test_block->addSuccessor(body_block);
        test_block->addSuccessor(end_block);
    }
    if (body_block) {
        body_block->addSuccessor(test_block);
    }
}

void IRGenerator::visitForStmtNode(ForStmtNode* node) {
    if (node->hasInit()) {
        node->getInit()->accept(this);
    }
    
    std::string test_label = newLabel();
    std::string body_label = newLabel();
    std::string update_label = newLabel();
    std::string end_label = newLabel();
    
    BasicBlock* entry_block = current_block;
    
    auto jump_to_test = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_test->target_label = test_label;
    emit(std::move(jump_to_test));
    
    auto label_test = std::make_unique<Instruction>(InstrKind::LABEL);
    label_test->dest = Operand::Label(test_label);
    emit(std::move(label_test));
    BasicBlock* test_block = current_block;
    
    if (node->hasCondition()) {
        Operand cond = generateExpression(node->getCondition());
        auto jump_if_not = std::make_unique<Instruction>(InstrKind::JUMP_IF_NOT);
        jump_if_not->src1 = cond;
        jump_if_not->target_label = end_label;
        emit(std::move(jump_if_not));
    }
    
    auto jump_to_body = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_body->target_label = body_label;
    emit(std::move(jump_to_body));
    
    auto label_body = std::make_unique<Instruction>(InstrKind::LABEL);
    label_body->dest = Operand::Label(body_label);
    emit(std::move(label_body));
    BasicBlock* body_block = current_block;
    
    node->getBody()->accept(this);
    
    auto jump_to_update = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_update->target_label = update_label;
    emit(std::move(jump_to_update));
    
    auto label_update = std::make_unique<Instruction>(InstrKind::LABEL);
    label_update->dest = Operand::Label(update_label);
    emit(std::move(label_update));
    BasicBlock* update_block = current_block;
    
    if (node->hasUpdate()) {
        node->getUpdate()->accept(this);
    }
    
    auto jump_back = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_back->target_label = test_label;
    emit(std::move(jump_back));
    
    auto label_end = std::make_unique<Instruction>(InstrKind::LABEL);
    label_end->dest = Operand::Label(end_label);
    emit(std::move(label_end));
    BasicBlock* end_block = current_block;
    
    if (entry_block) {
        entry_block->addSuccessor(test_block);
    }
    if (test_block) {
        test_block->addSuccessor(body_block);
        test_block->addSuccessor(end_block);
    }
    if (body_block) {
        body_block->addSuccessor(update_block);
    }
    if (update_block) {
        update_block->addSuccessor(test_block);
    }
}

void IRGenerator::visitReturnStmtNode(ReturnStmtNode* node) {
    if (node->hasValue()) {
        Operand value = generateExpression(node->getValue());
        auto instr = std::make_unique<Instruction>(InstrKind::RETURN);
        instr->src1 = value;
        emit(std::move(instr));
    } else {
        auto instr = std::make_unique<Instruction>(InstrKind::RETURN);
        emit(std::move(instr));
    }
}

void IRGenerator::visitVarDeclStmtNode(VarDeclStmtNode* node) {
    if (node->hasInitializer()) {
        Operand value = generateExpression(node->getInitializer());
        storeVariable(node->getName(), value);
    }
    
    if (current_function) {
        semantic::Type* var_type = nullptr;
        std::string type_name = node->getType();
        if (type_name == "int") {
            static semantic::Type int_type(semantic::TypeKind::INT);
            var_type = &int_type;
        } else if (type_name == "float") {
            static semantic::Type float_type(semantic::TypeKind::FLOAT);
            var_type = &float_type;
        } else if (type_name == "bool") {
            static semantic::Type bool_type(semantic::TypeKind::BOOL);
            var_type = &bool_type;
        }
        
        if (var_type) {
            current_function->addLocalVar(node->getName(), var_type);
        }
    }
}

void IRGenerator::visitExprStmtNode(ExprStmtNode* node) {
    if (node->getExpression()) {
        generateExpression(node->getExpression());
    }
}

// Объявления
void IRGenerator::visitFunctionDeclNode(FunctionDeclNode* node) {
    semantic::Type* ret_type = nullptr;
    std::string ret_type_str = node->getReturnType();
    
    if (ret_type_str == "int") {
        static semantic::Type int_type(semantic::TypeKind::INT);
        ret_type = &int_type;
    } else if (ret_type_str == "float") {
        static semantic::Type float_type(semantic::TypeKind::FLOAT);
        ret_type = &float_type;
    } else if (ret_type_str == "bool") {
        static semantic::Type bool_type(semantic::TypeKind::BOOL);
        ret_type = &bool_type;
    } else if (ret_type_str == "void") {
        static semantic::Type void_type(semantic::TypeKind::VOID);
        ret_type = &void_type;
    } else {
        static semantic::Type error_type(semantic::TypeKind::ERROR);
        ret_type = &error_type;
    }
    
    auto func = std::make_unique<IRFunction>(node->getName(), ret_type);
    current_function = func.get();
    
    for (const auto& param : node->getParameters()) {
        semantic::Type* param_type = nullptr;
        std::string param_type_str = param->getType();
        
        if (param_type_str == "int") {
            static semantic::Type int_type(semantic::TypeKind::INT);
            param_type = &int_type;
        } else if (param_type_str == "float") {
            static semantic::Type float_type(semantic::TypeKind::FLOAT);
            param_type = &float_type;
        } else if (param_type_str == "bool") {
            static semantic::Type bool_type(semantic::TypeKind::BOOL);
            param_type = &bool_type;
        } else {
            static semantic::Type error_type(semantic::TypeKind::ERROR);
            param_type = &error_type;
        }
        
        current_function->addParameter(param->getName(), param_type);
    }
    
    current_block = current_function->createBlock("entry");
    current_function->setEntryBlock(current_block);
    
    if (node->getBody()) {
        node->getBody()->accept(this);
    }
    
    program->addFunction(std::move(func));
    current_function = nullptr;
    current_block = nullptr;
}

void IRGenerator::visitStructDeclNode(StructDeclNode* node) {
    (void)node;
}

void IRGenerator::visitParamNode(ParamNode* node) {
    (void)node;
}

void IRGenerator::visitProgramNode(ProgramNode* node) {
    program = new IRProgram();
    
    for (const auto& decl : node->getDeclarations()) {
        decl->accept(this);
    }
}

IRProgram* IRGenerator::generate(ProgramNode* program_ast) {
    if (!program_ast) return nullptr;
    
    reset();
    program_ast->accept(this);
    return program;
}

void IRGenerator::reset() {
    program = nullptr;
    current_function = nullptr;
    current_block = nullptr;
    temp_counter = 0;
    label_counter = 0;
    
    while (!expr_stack.empty()) expr_stack.pop();
}

}
