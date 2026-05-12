#ifndef CODEGEN_X86_GENERATOR_HPP
#define CODEGEN_X86_GENERATOR_HPP

#include <string>
#include <sstream>
#include <memory>
#include <stack>
#include <unordered_map>
#include "../ir/ir.hpp"
#include "stack_frame.hpp"
#include "abi.hpp"
#include "register_allocator.hpp"

namespace codegen {

class X86Generator {
private:
    std::ostringstream out;
    std::ostringstream data_section;
    std::ostringstream bss_section;
    std::ostringstream rodata_section;
    
    ir::IRProgram* ir_program;
    const ir::IRFunction* current_function;
    StackFrame* current_frame;
    RegisterAllocator reg_alloc;
    
    std::unordered_map<std::string, std::string> var_mapping;
    
    int label_counter;
    int string_counter;
    
    bool current_block_ended;
    bool use_register_allocation;
    bool use_peephole_optimization;
    bool is_leaf_function;
    
    std::string last_call_dest;
    
    void emit(const std::string& asm_code);
    void emitLabel(const std::string& label);
    void emitComment(const std::string& comment);
    void emitDirective(const std::string& directive);
    
    std::string getOperand(const ir::Operand& op);
    std::string getDestOperand(const ir::Operand& dest, const ir::Operand& src);
    
    std::string fallbackGetOperand(const ir::Operand& op);
    
    void loadToReg(const std::string& reg, const ir::Operand& src);
    void storeFromReg(const std::string& reg, const ir::Operand& dest);
    
    bool isLeafFunction(const ir::IRFunction* func) const;
    void generatePrologue(const ir::IRFunction* func);
    void generateEpilogue(const ir::IRFunction* func);
    void generateInstruction(const ir::Instruction* instr);
    void generateArithmetic(const ir::Instruction* instr);
    void generateComparison(const ir::Instruction* instr);
    void generateLogic(const ir::Instruction* instr);
    void generateMemory(const ir::Instruction* instr);
    void generateControlFlow(const ir::Instruction* instr);
    void generateCall(const ir::Instruction* instr);
    void generateReturn(const ir::Instruction* instr);
    void generatePhi(const ir::Instruction* instr);
    void generateMove(const ir::Instruction* instr);
    
    void generateGlobalVariable(const std::string& name, const ir::Operand& value);
    void generateStringLiteral(const std::string& label, const std::string& value);
    
    static std::string extractBaseName(const std::string& ssa_name);
    
public:
    X86Generator();
    
    std::string generate(ir::IRProgram* program);
    
    void generateTextSection();
    void generateDataSection();
    void generateBssSection();
    void generateRodataSection();
    
    void generateFunction(const ir::IRFunction* func);
    
    void setRegisterAllocation(bool enable) { use_register_allocation = enable; }
    bool isRegisterAllocationEnabled() const { return use_register_allocation; }
    
    void setPeepholeOptimization(bool enable) { use_peephole_optimization = enable; }
    bool isPeepholeOptimizationEnabled() const { return use_peephole_optimization; }
};

}

#endif