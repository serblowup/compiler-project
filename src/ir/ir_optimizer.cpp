#include "ir_optimizer.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_set>

namespace ir {

// Конструктор
IROptimizer::IROptimizer()
    : original_instruction_count(0) {}

// Отчёт об оптимизациях
std::string IROptimizer::OptimizationReport::toString() const {
    std::ostringstream oss;
    oss << "Отчёт об оптимизации\n";
    
    int total_alg_simpl = add_zero + zero_add + mul_one + one_mul + mul_zero + zero_mul +
                          sub_zero + sub_self + div_one + cmp_gt_zero + cmp_eq_zero +
                          cmp_ne_zero + and_true + and_false + or_false + or_true;
    
    int total_const_fold = const_fold_arith + const_fold_logic + const_fold_not;
    
    oss << "Алгебраические упрощения:     " << total_alg_simpl << "\n";
    oss << "Свёртка констант:             " << total_const_fold << "\n";
    oss << "Упрощение силы операций:      " << mul_to_add << "\n";
    oss << "Удаление мёртвого кода:       " << (dead_move + dead_load + dead_arith) << "\n";
    oss << "Сцепление переходов:          " << (jump_to_next + jump_chain) << "\n";
    oss << "Удаление избыточных MOVE:     " << redundant_move << "\n\n";
    
    oss << "Метрики:\n";
    oss << "  Удалено инструкций:         " << total_instructions_removed << "\n";
    oss << "  Уменьшено временных:        " << temporaries_reduced << "\n";
    oss << "  Итераций оптимизации:       " << optimization_iterations << "\n";
    oss << "  Удалено строк:           " << std::fixed << std::setprecision(1) 
        << estimated_speedup << "%\n";
    
    return oss.str();
}

void IROptimizer::OptimizationReport::calculateMetrics(int original_instruction_count) {
    if (original_instruction_count > 0) {
        estimated_speedup = (static_cast<double>(total_instructions_removed) / 
                            original_instruction_count) * 100.0;
    }
}

// Основной метод оптимизации
void IROptimizer::optimize(IRProgram* program) {
    if (!program) return;
    
    // Сохраняем исходное состояние для метрик
    original_instruction_count = countInstructions(program);
    int original_temporaries = countTemporaries(program);
    
    report = OptimizationReport();
    int iteration = 0;
    
    for (const auto& func : program->getFunctions()) {
        optimizeFunction(func.get());
    }
    
    // Вычисляем метрики
    int new_temporaries = countTemporaries(program);
    report.temporaries_reduced = original_temporaries - new_temporaries;
    report.optimization_iterations = iteration;
    report.calculateMetrics(original_instruction_count);
}

void IROptimizer::optimizeFunction(IRFunction* func) {
    bool changed = true;
    int max_iterations = 10;
    int iteration = 0;
    
    for (iteration = 0; iteration < max_iterations && changed; ++iteration) {
        changed = false;
        
        // Собираем все блоки
        std::vector<BasicBlock*> blocks;
        for (auto& block : func->getBlocks()) {
            blocks.push_back(block.get());
        }
        
        for (BasicBlock* block : blocks) {
            auto& instrs = block->getInstructionsMutable();
            
            for (size_t i = 0; i < instrs.size(); ++i) {
                Instruction* instr = instrs[i].get();
                
                // Пропускаем LABEL
                if (instr->kind == InstrKind::LABEL) continue;
                
                // Применяем оптимизации по порядку
                if (constantFold(instr)) {
                    changed = true;
                    i--;
                    continue;
                }
                
                if (algebraicSimplify(instr)) {
                    changed = true;
                    i--;
                    continue;
                }
                
                if (strengthReduce(instr)) {
                    changed = true;
                    i--;
                    continue;
                }
                
                if (jumpChaining(block, i)) {
                    changed = true;
                    i--;
                    continue;
                }
                
                if (deadCodeEliminate(block, i)) {
                    changed = true;
                    i--;
                    continue;
                }
            }
        }
        
        if (eliminateRedundantMoves(func)) {
            changed = true;
        }
    }
    
    report.optimization_iterations += iteration;
}

// Вспомогательные методы для проверки констант
bool IROptimizer::isZero(const Operand& op) {
    if (op.kind == OperandKind::CONST_INT && op.int_value == 0) return true;
    if (op.kind == OperandKind::CONST_FLOAT && op.float_value == 0.0f) return true;
    return false;
}

bool IROptimizer::isOne(const Operand& op) {
    if (op.kind == OperandKind::CONST_INT && op.int_value == 1) return true;
    if (op.kind == OperandKind::CONST_FLOAT && op.float_value == 1.0f) return true;
    return false;
}

bool IROptimizer::isTwo(const Operand& op) {
    if (op.kind == OperandKind::CONST_INT && op.int_value == 2) return true;
    if (op.kind == OperandKind::CONST_FLOAT && op.float_value == 2.0f) return true;
    return false;
}

bool IROptimizer::isTrue(const Operand& op) {
    if (op.kind == OperandKind::CONST_BOOL && op.bool_value == true) return true;
    if (op.kind == OperandKind::CONST_INT && op.int_value != 0) return true;
    return false;
}

bool IROptimizer::isFalse(const Operand& op) {
    if (op.kind == OperandKind::CONST_BOOL && op.bool_value == false) return true;
    if (op.kind == OperandKind::CONST_INT && op.int_value == 0) return true;
    return false;
}

// Свёртка констант
bool IROptimizer::constantFold(Instruction* instr) {
    // Арифметическая свёртка: 3 + 4 → 7
    if (instr->kind == InstrKind::ADD ||
        instr->kind == InstrKind::SUB ||
        instr->kind == InstrKind::MUL ||
        instr->kind == InstrKind::DIV ||
        instr->kind == InstrKind::MOD) {
        
        if (instr->src1.isConstant() && instr->src2.isConstant()) {
            bool is_float = (instr->src1.kind == OperandKind::CONST_FLOAT ||
                             instr->src2.kind == OperandKind::CONST_FLOAT);
            
            if (is_float) {
                float left = (instr->src1.kind == OperandKind::CONST_FLOAT) 
                             ? instr->src1.float_value 
                             : static_cast<float>(instr->src1.int_value);
                float right = (instr->src2.kind == OperandKind::CONST_FLOAT)
                              ? instr->src2.float_value
                              : static_cast<float>(instr->src2.int_value);
                float result = 0.0f;
                
                switch (instr->kind) {
                    case InstrKind::ADD: result = left + right; break;
                    case InstrKind::SUB: result = left - right; break;
                    case InstrKind::MUL: result = left * right; break;
                    case InstrKind::DIV: if (right != 0.0f) result = left / right; break;
                    default: return false;
                }
                
                instr->kind = InstrKind::MOVE;
                instr->src1 = Operand::ConstFloat(result);
                instr->src2 = Operand::Temp("");
                report.const_fold_arith++;
                report.total_instructions_removed++;
                return true;
            } else {
                int left = instr->src1.int_value;
                int right = instr->src2.int_value;
                int result = 0;
                
                switch (instr->kind) {
                    case InstrKind::ADD: result = left + right; break;
                    case InstrKind::SUB: result = left - right; break;
                    case InstrKind::MUL: result = left * right; break;
                    case InstrKind::DIV: if (right != 0) result = left / right; break;
                    case InstrKind::MOD: if (right != 0) result = left % right; break;
                    default: return false;
                }
                
                instr->kind = InstrKind::MOVE;
                instr->src1 = Operand::ConstInt(result);
                instr->src2 = Operand::Temp("");
                report.const_fold_arith++;
                report.total_instructions_removed++;
                return true;
            }
        }
    }
    
    // Логическая свёртка: true && false → false
    if (instr->kind == InstrKind::AND || instr->kind == InstrKind::OR) {
        if (instr->src1.isConstant() && instr->src2.isConstant()) {
            bool left = isTrue(instr->src1);
            bool right = isTrue(instr->src2);
            bool result = (instr->kind == InstrKind::AND) ? (left && right) : (left || right);
            
            instr->kind = InstrKind::MOVE;
            instr->src1 = Operand::ConstBool(result);
            instr->src2 = Operand::Temp("");
            report.const_fold_logic++;
            report.total_instructions_removed++;
            return true;
        }
    }
    
    // NOT true → false, NOT false → true
    if (instr->kind == InstrKind::NOT && instr->src1.isConstant()) {
        bool val = isTrue(instr->src1);
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstBool(!val);
        instr->src2 = Operand::Temp("");
        report.const_fold_not++;
        report.total_instructions_removed++;
        return true;
    }
    
    return false;
}

// Алгебраические упрощения
bool IROptimizer::algebraicSimplify(Instruction* instr) {
    // x + 0 → x
    if (instr->kind == InstrKind::ADD && isZero(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.add_zero++;
        return true;
    }
    
    // 0 + x → x
    if (instr->kind == InstrKind::ADD && isZero(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src2;
        instr->src2 = Operand::Temp("");
        report.zero_add++;
        return true;
    }
    
    // x * 1 → x
    if (instr->kind == InstrKind::MUL && isOne(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.mul_one++;
        return true;
    }
    
    // 1 * x → x
    if (instr->kind == InstrKind::MUL && isOne(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src2;
        instr->src2 = Operand::Temp("");
        report.one_mul++;
        return true;
    }
    
    // x * 0 → 0
    if (instr->kind == InstrKind::MUL && isZero(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstInt(0);
        instr->src2 = Operand::Temp("");
        report.mul_zero++;
        report.total_instructions_removed++;
        return true;
    }
    
    // 0 * x → 0
    if (instr->kind == InstrKind::MUL && isZero(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstInt(0);
        instr->src2 = Operand::Temp("");
        report.zero_mul++;
        report.total_instructions_removed++;
        return true;
    }
    
    // x - 0 → x
    if (instr->kind == InstrKind::SUB && isZero(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.sub_zero++;
        return true;
    }
    
    // x - x → 0
    if (instr->kind == InstrKind::SUB && instr->src1.toString() == instr->src2.toString()) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstInt(0);
        instr->src2 = Operand::Temp("");
        report.sub_self++;
        report.total_instructions_removed++;
        return true;
    }
    
    // x / 1 → x
    if (instr->kind == InstrKind::DIV && isOne(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.div_one++;
        return true;
    }
    
    // CMP_GT x, 0 → x
    if (instr->kind == InstrKind::CMP_GT && isZero(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.cmp_gt_zero++;
        return true;
    }
    
    // CMP_EQ x, 0 → NOT x
    if (instr->kind == InstrKind::CMP_EQ && isZero(instr->src2)) {
        instr->kind = InstrKind::NOT;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.cmp_eq_zero++;
        return true;
    }
    
    // CMP_NE x, 0 → x
    if (instr->kind == InstrKind::CMP_NE && isZero(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.cmp_ne_zero++;
        return true;
    }
    
    // AND x, true → x
    if (instr->kind == InstrKind::AND && isTrue(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.and_true++;
        return true;
    }
    
    // AND true, x → x
    if (instr->kind == InstrKind::AND && isTrue(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src2;
        instr->src2 = Operand::Temp("");
        report.and_true++;
        return true;
    }
    
    // AND x, false → false
    if (instr->kind == InstrKind::AND && isFalse(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstBool(false);
        instr->src2 = Operand::Temp("");
        report.and_false++;
        report.total_instructions_removed++;
        return true;
    }
    
    // AND false, x → false
    if (instr->kind == InstrKind::AND && isFalse(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstBool(false);
        instr->src2 = Operand::Temp("");
        report.and_false++;
        report.total_instructions_removed++;
        return true;
    }
    
    // OR x, false → x
    if (instr->kind == InstrKind::OR && isFalse(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src1;
        instr->src2 = Operand::Temp("");
        report.or_false++;
        return true;
    }
    
    // OR false, x → x
    if (instr->kind == InstrKind::OR && isFalse(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = instr->src2;
        instr->src2 = Operand::Temp("");
        report.or_false++;
        return true;
    }
    
    // OR x, true → true
    if (instr->kind == InstrKind::OR && isTrue(instr->src2)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstBool(true);
        instr->src2 = Operand::Temp("");
        report.or_true++;
        report.total_instructions_removed++;
        return true;
    }
    
    // OR true, x → true
    if (instr->kind == InstrKind::OR && isTrue(instr->src1)) {
        instr->kind = InstrKind::MOVE;
        instr->src1 = Operand::ConstBool(true);
        instr->src2 = Operand::Temp("");
        report.or_true++;
        report.total_instructions_removed++;
        return true;
    }
    
    return false;
}

// Упрощение силы операций
bool IROptimizer::strengthReduce(Instruction* instr) {
    // x * 2 → x + x
    if (instr->kind == InstrKind::MUL && isTwo(instr->src2)) {
        instr->kind = InstrKind::ADD;
        instr->src2 = instr->src1;
        report.mul_to_add++;
        return true;
    }
    
    // 2 * x → x + x
    if (instr->kind == InstrKind::MUL && isTwo(instr->src1)) {
        instr->kind = InstrKind::ADD;
        instr->src1 = instr->src2;
        instr->src2 = instr->src2;
        report.mul_to_add++;
        return true;
    }
    
    return false;
}

// Удаление мёртвого кода
bool IROptimizer::isUsed(const std::string& temp_name, BasicBlock* block, size_t start_idx) {
    const auto& instrs = block->getInstructions();
    for (size_t i = start_idx + 1; i < instrs.size(); ++i) {
        const auto& instr = instrs[i];
        
        if (instr->dest.kind == OperandKind::TEMP && instr->dest.name == temp_name) return true;
        if (instr->src1.kind == OperandKind::TEMP && instr->src1.name == temp_name) return true;
        if (instr->src2.kind == OperandKind::TEMP && instr->src2.name == temp_name) return true;
        if ((instr->kind == InstrKind::JUMP_IF || instr->kind == InstrKind::JUMP_IF_NOT) &&
            instr->src1.kind == OperandKind::TEMP && instr->src1.name == temp_name) return true;
        for (const auto& arg : instr->args) {
            if (arg.kind == OperandKind::TEMP && arg.name == temp_name) return true;
        }
    }
    return false;
}

bool IROptimizer::deadCodeEliminate(BasicBlock* block, size_t idx) {
    auto& instrs = block->getInstructionsMutable();
    if (idx >= instrs.size()) return false;
    
    Instruction* instr = instrs[idx].get();
    
    // Не удаляем терминирующие инструкции
    if (instr->isTerminator()) return false;
    
    // Не удаляем STORE (имеет побочный эффект)
    if (instr->kind == InstrKind::STORE) return false;
    
    // MOVE с временной переменной
    if (instr->kind == InstrKind::MOVE && instr->dest.kind == OperandKind::TEMP) {
        if (!isUsed(instr->dest.name, block, idx)) {
            instrs.erase(instrs.begin() + idx);
            report.dead_move++;
            report.total_instructions_removed++;
            return true;
        }
    }
    
    // LOAD с временной переменной
    if (instr->kind == InstrKind::LOAD && instr->dest.kind == OperandKind::TEMP) {
        if (!isUsed(instr->dest.name, block, idx)) {
            instrs.erase(instrs.begin() + idx);
            report.dead_load++;
            report.total_instructions_removed++;
            return true;
        }
    }
    
    // Арифметические операции
    if ((instr->kind == InstrKind::ADD || instr->kind == InstrKind::SUB ||
         instr->kind == InstrKind::MUL || instr->kind == InstrKind::DIV ||
         instr->kind == InstrKind::MOD || instr->kind == InstrKind::CMP_EQ ||
         instr->kind == InstrKind::CMP_NE || instr->kind == InstrKind::CMP_LT ||
         instr->kind == InstrKind::CMP_LE || instr->kind == InstrKind::CMP_GT ||
         instr->kind == InstrKind::CMP_GE || instr->kind == InstrKind::NOT ||
         instr->kind == InstrKind::AND || instr->kind == InstrKind::OR) && 
        instr->dest.kind == OperandKind::TEMP) {
        
        if (!isUsed(instr->dest.name, block, idx)) {
            instrs.erase(instrs.begin() + idx);
            report.dead_arith++;
            report.total_instructions_removed++;
            return true;
        }
    }
    
    return false;
}

// Сцепление переходов
bool IROptimizer::jumpChaining(BasicBlock* block, size_t idx) {
    auto& instrs = block->getInstructionsMutable();
    if (idx + 1 >= instrs.size()) return false;
    
    Instruction* curr = instrs[idx].get();
    Instruction* next = instrs[idx + 1].get();
    
    // JUMP L1; LABEL L1: → удаляем JUMP
    if (curr->kind == InstrKind::JUMP && next->kind == InstrKind::LABEL) {
        if (curr->target_label == next->dest.name) {
            instrs.erase(instrs.begin() + idx);
            report.jump_to_next++;
            report.total_instructions_removed++;
            return true;
        }
    }
    
    // JUMP L1; JUMP L2 → заменяем на JUMP L2
    if (curr->kind == InstrKind::JUMP && next->kind == InstrKind::JUMP) {
        curr->target_label = next->target_label;
        instrs.erase(instrs.begin() + idx + 1);
        report.jump_chain++;
        report.total_instructions_removed++;
        return true;
    }
    
    return false;
}

// Удаление избыточных MOVE
void IROptimizer::replaceAllUses(const std::string& old_temp, const std::string& new_temp,
                                  BasicBlock* block, size_t start_idx) {
    auto& instrs = block->getInstructionsMutable();
    for (size_t i = start_idx + 1; i < instrs.size(); ++i) {
        Instruction* instr = instrs[i].get();
        
        if (instr->dest.kind == OperandKind::TEMP && instr->dest.name == old_temp) {
            instr->dest.name = new_temp;
        }
        if (instr->src1.kind == OperandKind::TEMP && instr->src1.name == old_temp) {
            instr->src1.name = new_temp;
        }
        if (instr->src2.kind == OperandKind::TEMP && instr->src2.name == old_temp) {
            instr->src2.name = new_temp;
        }
        if ((instr->kind == InstrKind::JUMP_IF || instr->kind == InstrKind::JUMP_IF_NOT) &&
            instr->src1.kind == OperandKind::TEMP && instr->src1.name == old_temp) {
            instr->src1.name = new_temp;
        }
        for (auto& arg : instr->args) {
            if (arg.kind == OperandKind::TEMP && arg.name == old_temp) {
                arg.name = new_temp;
            }
        }
    }
}

bool IROptimizer::eliminateRedundantMoves(IRFunction* func) {
    bool changed = false;
    
    for (auto& block : func->getBlocks()) {
        auto& instrs = block->getInstructionsMutable();
        
        for (size_t i = 0; i < instrs.size(); ++i) {
            Instruction* instr = instrs[i].get();
            
            // MOVE t_dest, t_src
            if (instr->kind == InstrKind::MOVE && 
                instr->dest.kind == OperandKind::TEMP &&
                instr->src1.kind == OperandKind::TEMP &&
                instr->dest.name != instr->src1.name) {
                
                std::string dest_name = instr->dest.name;
                std::string src_name = instr->src1.name;
                
                // Заменяем все использования dest_name на src_name
                replaceAllUses(dest_name, src_name, block.get(), i);
                
                // Удаляем избыточный MOVE
                instrs.erase(instrs.begin() + i);
                changed = true;
                report.redundant_move++;
                report.total_instructions_removed++;
                i--;
            }
        }
    }
    
    return changed;
}

// Подсчёт метрик
int IROptimizer::countInstructions(IRProgram* program) {
    int count = 0;
    for (const auto& func : program->getFunctions()) {
        for (const auto& block : func->getBlocks()) {
            count += block->getInstructions().size();
        }
    }
    return count;
}

int IROptimizer::countTemporaries(IRProgram* program) {
    std::unordered_set<std::string> temps;
    for (const auto& func : program->getFunctions()) {
        for (const auto& block : func->getBlocks()) {
            for (const auto& instr : block->getInstructions()) {
                if (instr->dest.kind == OperandKind::TEMP) temps.insert(instr->dest.name);
                if (instr->src1.kind == OperandKind::TEMP) temps.insert(instr->src1.name);
                if (instr->src2.kind == OperandKind::TEMP) temps.insert(instr->src2.name);
            }
        }
    }
    return temps.size();
}

const IROptimizer::OptimizationReport& IROptimizer::getReport() const {
    return report;
}

}
