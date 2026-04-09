#ifndef IR_OPTIMIZER_HPP
#define IR_OPTIMIZER_HPP

#include "ir.hpp"
#include <memory>
#include <vector>
#include <string>

namespace ir {

// Оптимизации IR
class IROptimizer {
public:
    // Отчёт об оптимизации
    struct OptimizationReport {
        // Алгебраические упрощения
        int add_zero = 0;
        int zero_add = 0;
        int mul_one = 0;
        int one_mul = 0;
        int mul_zero = 0;
        int zero_mul = 0;
        int sub_zero = 0;
        int sub_self = 0;
        int div_one = 0;
        int cmp_gt_zero = 0;
        int cmp_eq_zero = 0;
        int cmp_ne_zero = 0;
        int and_true = 0;
        int and_false = 0;
        int or_false = 0;
        int or_true = 0;
        
        // Свёртка констант
        int const_fold_arith = 0;
        int const_fold_logic = 0;
        int const_fold_not = 0;
        
        // Уменьшение силы операций
        int mul_to_add = 0;
        
        // Удаление мёртвого кода
        int dead_move = 0;
        int dead_load = 0;
        int dead_arith = 0;
        
        // Сцепление переходов
        int jump_to_next = 0;
        int jump_chain = 0;
        
        // Удаление избыточных MOVE
        int redundant_move = 0;
        
        // SSA-специфичные оптимизации
        int const_propagation = 0;
        int copy_propagation = 0;
        int redundant_phis = 0;
        int unreachable_code = 0;
        
        // Метрики
        int total_instructions_removed = 0;
        int temporaries_reduced = 0;
        int optimization_iterations = 0;
        double estimated_speedup = 0.0;
        
        std::string toString() const;
        void calculateMetrics(int original_instruction_count);
    };
    
    IROptimizer();
    
    void optimize(IRProgram* program);
    void optimizeSSA(IRProgram* program);
    
    const OptimizationReport& getReport() const;
    
private:
    OptimizationReport report;
    int original_instruction_count;
    
    void optimizeFunction(IRFunction* func);
    void optimizeSSAFunction(IRFunction* func);
    
    // Оптимизации
    bool algebraicSimplify(Instruction* instr);
    bool constantFold(Instruction* instr);
    bool strengthReduce(Instruction* instr);
    bool deadCodeEliminate(BasicBlock* block, size_t idx);
    bool jumpChaining(BasicBlock* block, size_t idx);
    bool eliminateRedundantMoves(IRFunction* func);
    
    // SSA-специфичные оптимизации
    bool constantPropagation(IRFunction* func);
    bool copyPropagation(IRFunction* func);
    bool eliminateRedundantPhis(IRFunction* func);
    bool removeUnreachableBlocks(IRFunction* func);
    
    // Вспомогательные методы
    bool isZero(const Operand& op);
    bool isOne(const Operand& op);
    bool isTwo(const Operand& op);
    bool isTrue(const Operand& op);
    bool isFalse(const Operand& op);
    bool isUsed(const std::string& temp_name, BasicBlock* block, size_t start_idx);
    void replaceAllUses(const std::string& old_temp, const std::string& new_temp,
                        BasicBlock* block, size_t start_idx);
    bool isConstantValue(const Operand& op, int& value);
    
    // Подсчёт метрик
    int countInstructions(IRProgram* program);
    int countTemporaries(IRProgram* program);
};

}

#endif