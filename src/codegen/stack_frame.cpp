#include "stack_frame.hpp"
#include "abi.hpp"
#include <sstream>
#include <set>
#include <algorithm>

namespace codegen {

StackFrame::StackFrame(const std::string& name)
    : function_name(name)
    , total_stack_size(0)
    , local_vars_size(0)
    , saved_regs_size(0)
    , spill_slots_size(0)
    , current_offset(0) {
    local_vars.clear();
    saved_registers.clear();
}

static int align_value(int value, int alignment) {
    if (alignment <= 0) return value;
    int remainder = value % alignment;
    if (remainder == 0) return value;
    return value + (alignment - remainder);
}

int StackFrame::allocateVar(const std::string& name, int size, int alignment) {
    current_offset = align_value(current_offset, alignment);
    current_offset += size;
    local_vars_size = current_offset;
    
    int offset = -current_offset;
    local_vars.push_back(StackVar(name, offset, size, alignment));
    
    return offset;
}

int StackFrame::getVarOffset(const std::string& name) const {
    for (const auto& var : local_vars) {
        if (var.name == name) {
            return var.offset;
        }
    }
    return 0;
}

void StackFrame::saveRegister(const std::string& reg) {
    saved_registers.push_back(reg);
    saved_regs_size += ABI::REGISTER_SIZE;
}

void StackFrame::setSpillSlotsSize(int size) {
    spill_slots_size = size;
}

int StackFrame::getSpillSlotsSize() const {
    return spill_slots_size;
}

int StackFrame::getTotalStackSize() const {
    int total = local_vars_size;
    
    // Добавляем spill-слоты только если они есть
    if (spill_slots_size > 0) {
        total += spill_slots_size;
    }
    
    // Выравниваем до 16 байт
    total = align_value(total, ABI::STACK_ALIGNMENT);
    
    // Минимальный размер стека — 16 байт (для пустых функций)
    if (total < 16) {
        total = 16;
    }
    
    return total;
}

int StackFrame::getLocalVarsSize() const {
    return local_vars_size;
}

int StackFrame::getSavedRegsSize() const {
    return saved_regs_size;
}

const std::vector<std::string>& StackFrame::getSavedRegisters() const {
    return saved_registers;
}

const std::vector<StackVar>& StackFrame::getLocalVars() const {
    return local_vars;
}

bool StackFrame::hasVar(const std::string& name) const {
    for (const auto& var : local_vars) {
        if (var.name == name) {
            return true;
        }
    }
    return false;
}

std::string StackFrame::toString() const {
    std::ostringstream oss;
    oss << "StackFrame for " << function_name << ":\n";
    oss << "  Local vars size: " << local_vars_size << " bytes\n";
    oss << "  Saved regs size: " << saved_regs_size << " bytes\n";
    oss << "  Spill slots size: " << spill_slots_size << " bytes\n";
    oss << "  Total stack size: " << getTotalStackSize() << " bytes\n";
    oss << "  Variables:\n";
    for (const auto& var : local_vars) {
        oss << "    " << var.name << ": offset " << var.offset 
            << ", size " << var.size << " (alignment " << var.alignment << ")\n";
    }
    oss << "  Saved registers:\n";
    for (const auto& reg : saved_registers) {
        oss << "    " << reg << "\n";
    }
    return oss.str();
}

StackFrame StackFrameAnalyzer::analyze(const ir::IRFunction* func) {
    StackFrame frame(func->getName());
    
    for (const auto& param : func->getParameters()) {
        int size = ABI::getTypeSize(param.type);
        int alignment = ABI::getTypeAlignment(param.type);
        frame.allocateVar(param.name, size, alignment);
    }
    
    for (const auto& var : func->getLocalVars()) {
        int size = ABI::getTypeSize(var.type);
        int alignment = ABI::getTypeAlignment(var.type);
        frame.allocateVar(var.name, size, alignment);
    }
    
    auto used_regs = getUsedCalleeSavedRegs(func);
    for (const auto& reg : used_regs) {
        frame.saveRegister(reg);
    }
    
    return frame;
}

std::vector<std::string> StackFrameAnalyzer::getUsedCalleeSavedRegs(
    const ir::IRFunction* func) {
    
    std::set<std::string> used_regs;

    size_t instruction_count = 0;
    for (const auto& block : func->getBlocks()) {
        instruction_count += block->getInstructions().size();
    }
    
    if (instruction_count > 10 || func->getLocalVars().size() > 4) {
        used_regs.insert("rbx");
        used_regs.insert("r12");
        used_regs.insert("r13");
    }
    
    return std::vector<std::string>(used_regs.begin(), used_regs.end());
}

}