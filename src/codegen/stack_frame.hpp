#ifndef CODEGEN_STACK_FRAME_HPP
#define CODEGEN_STACK_FRAME_HPP

#include <string>
#include <vector>
#include "../ir/ir.hpp"

namespace codegen {

struct StackVar {
    std::string name;
    int offset;
    int size;
    int alignment;
    
    StackVar(const std::string& n, int off, int sz, int al)
        : name(n), offset(off), size(sz), alignment(al) {}
};

class StackFrame {
private:
    std::string function_name;
    int total_stack_size;
    int local_vars_size;
    int saved_regs_size;
    int spill_slots_size;
    std::vector<std::string> saved_registers;
    std::vector<StackVar> local_vars;
    int current_offset;
    
public:
    StackFrame(const std::string& name);
    
    // Выделение переменных
    int allocateVar(const std::string& name, int size, int alignment);
    int getVarOffset(const std::string& name) const;
    
    // Сохранение регистров
    void saveRegister(const std::string& reg);
    
    void setSpillSlotsSize(int size);
    int getSpillSlotsSize() const;
    
    // Размеры
    int getTotalStackSize() const;
    int getLocalVarsSize() const;
    int getSavedRegsSize() const;
    
    // Доступ к данным
    const std::vector<std::string>& getSavedRegisters() const;
    const std::vector<StackVar>& getLocalVars() const;
    bool hasVar(const std::string& name) const;
    
    // Отладка
    std::string toString() const;
};

class StackFrameAnalyzer {
public:
    static StackFrame analyze(const ir::IRFunction* func);
    static std::vector<std::string> getUsedCalleeSavedRegs(const ir::IRFunction* func);
};

}

#endif