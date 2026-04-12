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

// SSA: получение текущей версии переменной
std::string IRGenerator::getCurrentVersion(const std::string& var) {
    auto it = version_stacks.find(var);
    if (it == version_stacks.end() || it->second.empty()) {
        return var + "_0";
    }
    return it->second.top();
}

// SSA: создание новой версии переменной
std::string IRGenerator::newVersion(const std::string& var) {
    int version = ++var_versions[var];
    return var + "_" + std::to_string(version);
}

// SSA: сохранить новую версию
void IRGenerator::pushVersion(const std::string& var) {
    std::string version = newVersion(var);
    version_stacks[var].push(version);
}

// SSA: восстановить предыдущую версию
void IRGenerator::popVersion(const std::string& var) {
    auto it = version_stacks.find(var);
    if (it != version_stacks.end() && !it->second.empty()) {
        it->second.pop();
    }
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

// SSA: загрузка переменной
Operand IRGenerator::loadVariable(const std::string& name, semantic::Type* type) {
    (void)type;
    std::string versioned_name = getCurrentVersion(name);
    return Operand::Temp(versioned_name);
}

// SSA: сохранение переменной
void IRGenerator::storeVariable(const std::string& name, const Operand& value) {
    pushVersion(name);
    std::string versioned_name = getCurrentVersion(name);
    
    auto instr = std::make_unique<Instruction>(InstrKind::MOVE);
    instr->dest = Operand::Temp(versioned_name);
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

// SSA: присваивание создаёт новую версию переменной
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
    instr->src1 = Operand::Label(func_name);
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

// SSA: if с Phi-функциями на выходе
void IRGenerator::visitIfStmtNode(IfStmtNode* node) {
    std::string then_label = newLabel();
    std::string else_label = newLabel();
    std::string end_label = newLabel();
    
    BasicBlock* entry_block = current_block;
    
    // Запоминаем версии перед условием
    auto before_if_stacks = version_stacks;
    
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
    
    // Then branch
    auto label_then = std::make_unique<Instruction>(InstrKind::LABEL);
    label_then->dest = Operand::Label(then_label);
    emit(std::move(label_then));
    then_block = current_block;
    
    // Сохраняем стеки версий перед then-веткой
    auto then_stacks = version_stacks;
    
    // Флаг, была ли инструкция RETURN в then-ветке
    bool then_has_return = false;
    
    node->getThenBranch()->accept(this);
    
    // Проверяем, заканчивается ли then-ветка RETURN
    if (!current_block->getInstructions().empty()) {
        auto& last_instr = current_block->getInstructions().back();
        if (last_instr->kind == InstrKind::RETURN) {
            then_has_return = true;
        }
    }
    
    // Добавляем JUMP к end_label только если then-ветка не заканчивается RETURN
    if (!then_has_return) {
        auto jump_to_end = std::make_unique<Instruction>(InstrKind::JUMP);
        jump_to_end->target_label = end_label;
        emit(std::move(jump_to_end));
    }
    
    // Else branch
    auto label_else = std::make_unique<Instruction>(InstrKind::LABEL);
    label_else->dest = Operand::Label(else_label);
    emit(std::move(label_else));
    else_block = current_block;
    
    // Восстанавливаем стеки для else-ветки
    version_stacks = then_stacks;
    
    // Флаг, была ли инструкция RETURN в else-ветке
    bool else_has_return = false;
    
    if (node->hasElse()) {
        node->getElseBranch()->accept(this);
        
        // Проверяем, заканчивается ли else-ветка RETURN
        if (!current_block->getInstructions().empty()) {
            auto& last_instr = current_block->getInstructions().back();
            if (last_instr->kind == InstrKind::RETURN) {
                else_has_return = true;
            }
        }
    }
    
    // Добавляем JUMP к end_label только если else-ветка не заканчивается RETURN
    if (!else_has_return) {
        auto jump_to_end2 = std::make_unique<Instruction>(InstrKind::JUMP);
        jump_to_end2->target_label = end_label;
        emit(std::move(jump_to_end2));
    }
    
    if (then_has_return && else_has_return) {
        if (entry_block) {
            entry_block->addSuccessor(then_block);
            entry_block->addSuccessor(else_block);
        }
        return;
    }
    
    // End block
    auto label_end = std::make_unique<Instruction>(InstrKind::LABEL);
    label_end->dest = Operand::Label(end_label);
    emit(std::move(label_end));
    BasicBlock* end_block = current_block;
    
    // Создаём Phi-функции для переменных, которые изменились
    std::set<std::string> all_vars;
    for (const auto& [var, _] : then_stacks) {
        all_vars.insert(var);
    }
    for (const auto& [var, _] : version_stacks) {
        all_vars.insert(var);
    }
    
    for (const std::string& var : all_vars) {
        std::string then_version = "undefined";
        std::string else_version = "undefined";
        
        auto it_then = then_stacks.find(var);
        if (it_then != then_stacks.end() && !it_then->second.empty()) {
            then_version = it_then->second.top();
        } else {
            auto it_saved = before_if_stacks.find(var);
            if (it_saved != before_if_stacks.end() && !it_saved->second.empty()) {
                then_version = it_saved->second.top();
            }
        }
        
        auto it_else = version_stacks.find(var);
        if (it_else != version_stacks.end() && !it_else->second.empty()) {
            else_version = it_else->second.top();
        } else {
            auto it_saved = before_if_stacks.find(var);
            if (it_saved != before_if_stacks.end() && !it_saved->second.empty()) {
                else_version = it_saved->second.top();
            }
        }
        
        // Если версии различаются, нужна Phi
        if (then_version != else_version && then_version != "undefined" && else_version != "undefined") {
            pushVersion(var);
            std::string phi_result = getCurrentVersion(var);
            
            auto phi_instr = std::make_unique<Instruction>(InstrKind::PHI);
            phi_instr->dest = Operand::Temp(phi_result);
            phi_instr->args.push_back(Operand::Temp(then_version));
            phi_instr->args.push_back(Operand::Label(then_label));
            phi_instr->args.push_back(Operand::Temp(else_version));
            phi_instr->args.push_back(Operand::Label(else_label));
            
            // Вставляем в начало end_block
            end_block->getInstructionsMutable().insert(
                end_block->getInstructionsMutable().begin(),
                std::move(phi_instr)
            );
        }
    }
    
    // Обновляем связи CFG
    if (entry_block) {
        entry_block->addSuccessor(then_block);
        entry_block->addSuccessor(else_block);
    }
    if (then_block && !then_has_return) {
        then_block->addSuccessor(end_block);
    }
    if (else_block && !else_has_return) {
        else_block->addSuccessor(end_block);
    }
}

// SSA: while с Phi-функциями
void IRGenerator::visitWhileStmtNode(WhileStmtNode* node) {
    std::string header_label = newLabel();
    std::string body_label = newLabel();
    std::string exit_label = newLabel();
    
    BasicBlock* entry_block = current_block;
    
    // Запоминаем версии перед циклом
    auto before_loop_stacks = version_stacks;
    
    // Прыгаем в заголовок
    auto jump_to_header = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_header->target_label = header_label;
    emit(std::move(jump_to_header));
    
    // Заголовок цикла (блок с Phi-функциями)
    auto label_header = std::make_unique<Instruction>(InstrKind::LABEL);
    label_header->dest = Operand::Label(header_label);
    emit(std::move(label_header));
    BasicBlock* header_block = current_block;
    
    std::set<std::string> loop_vars;
    for (const auto& [var, stack] : version_stacks) {
        bool is_param = false;
        for (const auto& param : current_function->getParameters()) {
            if (param.name == var) {
                is_param = true;
                break;
            }
        }
        if (!is_param && !stack.empty()) {
            loop_vars.insert(var);
        }
    }
    
    // Сохраняем Phi-версии для использования после цикла
    std::unordered_map<std::string, std::string> phi_versions;
    
    // Создаём Phi-функции в заголовке
    for (const std::string& var : loop_vars) {
        std::string pre_version = var + "_0";
        auto it_pre = before_loop_stacks.find(var);
        if (it_pre != before_loop_stacks.end() && !it_pre->second.empty()) {
            pre_version = it_pre->second.top();
        }
        
        pushVersion(var);
        std::string phi_version = getCurrentVersion(var);
        phi_versions[var] = phi_version;
        
        // Создаем Phi-инструкцию
        auto phi_instr = std::make_unique<Instruction>(InstrKind::PHI);
        phi_instr->dest = Operand::Temp(phi_version);
        phi_instr->args.push_back(Operand::Temp(pre_version));
        phi_instr->args.push_back(Operand::Label("entry"));
        phi_instr->args.push_back(Operand::Temp("placeholder"));
        phi_instr->args.push_back(Operand::Label(body_label));
        
        header_block->getInstructionsMutable().push_back(std::move(phi_instr));
    }
    
    for (const auto& [var, phi_ver] : phi_versions) {
        while (!version_stacks[var].empty()) {
            version_stacks[var].pop();
        }
        version_stacks[var].push(phi_ver);
    }
    
    // Генерируем условие
    Operand cond = generateExpression(node->getCondition());
    
    auto jump_if_not = std::make_unique<Instruction>(InstrKind::JUMP_IF_NOT);
    jump_if_not->src1 = cond;
    jump_if_not->target_label = exit_label;
    emit(std::move(jump_if_not));
    
    auto jump_to_body = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_body->target_label = body_label;
    emit(std::move(jump_to_body));
    
    // Тело цикла
    auto label_body = std::make_unique<Instruction>(InstrKind::LABEL);
    label_body->dest = Operand::Label(body_label);
    emit(std::move(label_body));
    BasicBlock* body_block = current_block;
    
    node->getBody()->accept(this);
    
    // Запоминаем версии после тела
    auto after_body_stacks = version_stacks;
    
    // Прыжок обратно в заголовок
    auto jump_back = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_back->target_label = header_label;
    emit(std::move(jump_back));
    
    auto& header_instrs = header_block->getInstructionsMutable();
    for (auto& instr : header_instrs) {
        if (instr->kind == InstrKind::PHI) {
            std::string phi_dest = instr->dest.name;
            size_t underscore = phi_dest.rfind('_');
            std::string base_var = (underscore != std::string::npos) ? phi_dest.substr(0, underscore) : phi_dest;
            
            // Получаем версию из тела
            std::string body_version = base_var + "_0";
            auto it_body = after_body_stacks.find(base_var);
            if (it_body != after_body_stacks.end() && !it_body->second.empty()) {
                body_version = it_body->second.top();
            }
            
            // Обновляем второй аргумент Phi
            if (instr->args.size() >= 4) {
                instr->args[2] = Operand::Temp(body_version);
            }
        }
    }
    
    // Exit блок
    auto label_exit = std::make_unique<Instruction>(InstrKind::LABEL);
    label_exit->dest = Operand::Label(exit_label);
    emit(std::move(label_exit));
    BasicBlock* exit_block = current_block;

    for (const std::string& var : loop_vars) {
        auto it_phi = phi_versions.find(var);
        if (it_phi != phi_versions.end()) {
            while (!version_stacks[var].empty()) {
                version_stacks[var].pop();
            }
            version_stacks[var].push(it_phi->second);
        }
    }
    
    // Обновляем связи CFG
    if (entry_block) {
        entry_block->addSuccessor(header_block);
    }
    if (header_block) {
        header_block->addSuccessor(body_block);
        header_block->addSuccessor(exit_block);
    }
    if (body_block) {
        body_block->addSuccessor(header_block);
    }
}

// SSA: for с Phi-функциями
void IRGenerator::visitForStmtNode(ForStmtNode* node) {
    // Инициализация
    if (node->hasInit()) {
        node->getInit()->accept(this);
    }
    
    std::string header_label = newLabel();
    std::string body_label = newLabel();
    std::string update_label = newLabel();
    std::string exit_label = newLabel();
    
    BasicBlock* entry_block = current_block;
    
    // Запоминаем версии перед циклом
    auto before_loop_stacks = version_stacks;
    
    // Прыгаем в заголовок
    auto jump_to_header = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_header->target_label = header_label;
    emit(std::move(jump_to_header));
    
    // Заголовок цикла
    auto label_header = std::make_unique<Instruction>(InstrKind::LABEL);
    label_header->dest = Operand::Label(header_label);
    emit(std::move(label_header));
    BasicBlock* header_block = current_block;
    
    std::set<std::string> loop_vars;
    for (const auto& [var, stack] : version_stacks) {
        bool is_param = false;
        for (const auto& param : current_function->getParameters()) {
            if (param.name == var) {
                is_param = true;
                break;
            }
        }
        if (!is_param && !stack.empty()) {
            loop_vars.insert(var);
        }
    }
    
    std::unordered_map<std::string, std::string> phi_versions;
    
    for (const std::string& var : loop_vars) {
        std::string pre_version = var + "_0";
        auto it_pre = before_loop_stacks.find(var);
        if (it_pre != before_loop_stacks.end() && !it_pre->second.empty()) {
            pre_version = it_pre->second.top();
        }
        
        pushVersion(var);
        std::string phi_version = getCurrentVersion(var);
        phi_versions[var] = phi_version;
        
        auto phi_instr = std::make_unique<Instruction>(InstrKind::PHI);
        phi_instr->dest = Operand::Temp(phi_version);
        phi_instr->args.push_back(Operand::Temp(pre_version));
        phi_instr->args.push_back(Operand::Label("entry"));
        phi_instr->args.push_back(Operand::Temp("placeholder"));
        phi_instr->args.push_back(Operand::Label(update_label));
        
        header_block->getInstructionsMutable().push_back(std::move(phi_instr));
    }
    
    for (const auto& [var, phi_ver] : phi_versions) {
        while (!version_stacks[var].empty()) {
            version_stacks[var].pop();
        }
        version_stacks[var].push(phi_ver);
    }
    
    // Проверка условия
    if (node->hasCondition()) {
        Operand cond = generateExpression(node->getCondition());
        auto jump_if_not = std::make_unique<Instruction>(InstrKind::JUMP_IF_NOT);
        jump_if_not->src1 = cond;
        jump_if_not->target_label = exit_label;
        emit(std::move(jump_if_not));
    }
    
    auto jump_to_body = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_body->target_label = body_label;
    emit(std::move(jump_to_body));
    
    // Тело цикла
    auto label_body = std::make_unique<Instruction>(InstrKind::LABEL);
    label_body->dest = Operand::Label(body_label);
    emit(std::move(label_body));
    BasicBlock* body_block = current_block;
    
    node->getBody()->accept(this);
    
    auto jump_to_update = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_to_update->target_label = update_label;
    emit(std::move(jump_to_update));
    
    // Блок обновления
    auto label_update = std::make_unique<Instruction>(InstrKind::LABEL);
    label_update->dest = Operand::Label(update_label);
    emit(std::move(label_update));
    BasicBlock* update_block = current_block;
    
    if (node->hasUpdate()) {
        node->getUpdate()->accept(this);
    }
    
    auto after_update_stacks = version_stacks;
    
    auto jump_back = std::make_unique<Instruction>(InstrKind::JUMP);
    jump_back->target_label = header_label;
    emit(std::move(jump_back));
    
    auto& header_instrs = header_block->getInstructionsMutable();
    for (auto& instr : header_instrs) {
        if (instr->kind == InstrKind::PHI) {
            std::string phi_dest = instr->dest.name;
            size_t underscore = phi_dest.rfind('_');
            std::string base_var = (underscore != std::string::npos) ? phi_dest.substr(0, underscore) : phi_dest;
            
            std::string update_version = base_var + "_0";
            auto it_update = after_update_stacks.find(base_var);
            if (it_update != after_update_stacks.end() && !it_update->second.empty()) {
                update_version = it_update->second.top();
            }
            
            if (instr->args.size() >= 4) {
                instr->args[2] = Operand::Temp(update_version);
            }
        }
    }
    
    // Exit блок
    auto label_exit = std::make_unique<Instruction>(InstrKind::LABEL);
    label_exit->dest = Operand::Label(exit_label);
    emit(std::move(label_exit));
    BasicBlock* exit_block = current_block;
    
    for (const std::string& var : loop_vars) {
        auto it_phi = phi_versions.find(var);
        if (it_phi != phi_versions.end()) {
            while (!version_stacks[var].empty()) {
                version_stacks[var].pop();
            }
            version_stacks[var].push(it_phi->second);
        }
    }
    
    // Обновляем связи CFG
    if (entry_block) {
        entry_block->addSuccessor(header_block);
    }
    if (header_block) {
        header_block->addSuccessor(body_block);
        header_block->addSuccessor(exit_block);
    }
    if (body_block) {
        body_block->addSuccessor(update_block);
    }
    if (update_block) {
        update_block->addSuccessor(header_block);
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
    std::string var_name = node->getName();
    
    // Инициализация переменной
    if (node->hasInitializer()) {
        Operand value = generateExpression(node->getInitializer());
        storeVariable(var_name, value);
    } else {
        pushVersion(var_name);
        std::string versioned_name = getCurrentVersion(var_name);
        
        auto instr = std::make_unique<Instruction>(InstrKind::MOVE);
        instr->dest = Operand::Temp(versioned_name);
        instr->src1 = Operand::ConstInt(0);
        emit(std::move(instr));
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
    
    // Сброс SSA-состояния для новой функции
    var_versions.clear();
    version_stacks.clear();
    
    // Создаём entry блок
    current_block = current_function->createBlock("entry");
    current_function->setEntryBlock(current_block);
    
    // Добавляем параметры с правильной инициализацией
    int param_index = 0;
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
        
        // Создаём версию _0 для параметра
        pushVersion(param->getName());
        std::string versioned_name = getCurrentVersion(param->getName());
        
        // Добавляем PARAM инструкцию
        auto param_instr = std::make_unique<Instruction>(InstrKind::PARAM);
        param_instr->src1 = Operand::ConstInt(param_index++);
        param_instr->src2 = Operand::Temp(versioned_name);
        current_block->addInstruction(std::move(param_instr));
    }

    // Генерируем тело функции
    if (node->getBody()) {
        node->getBody()->accept(this);
    }
    
    // Добавляем неявный RETURN для void функций
    if (ret_type_str == "void") {
        auto ret_instr = std::make_unique<Instruction>(InstrKind::RETURN);
        current_block->addInstruction(std::move(ret_instr));
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
    var_versions.clear();
    version_stacks.clear();
    
    while (!expr_stack.empty()) expr_stack.pop();
    while (!short_circuit_stack.empty()) short_circuit_stack.pop();
}

}
