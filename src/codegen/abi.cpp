#include "abi.hpp"
#include "../ir/ir.hpp"
#include <algorithm>
#include <iostream>
#include <cctype>

namespace codegen {

const std::vector<std::string> ABI::ARG_REGISTERS_INT = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

const std::vector<std::string> ABI::ARG_REGISTERS_FLOAT = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
};

const std::vector<std::string> ABI::CALLER_SAVED = {
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"
};

const std::vector<std::string> ABI::CALLEE_SAVED = {
    "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15"
};

const std::string ABI::RETURN_REG_INT = "rax";
const std::string ABI::RETURN_REG_FLOAT = "xmm0";
const std::string ABI::STACK_PTR = "rsp";
const std::string ABI::BASE_PTR = "rbp";

int ABI::getTypeSize(semantic::Type* type) {
    if (!type) return 4;
    
    switch (type->getKind()) {
        case semantic::TypeKind::INT:    return 4;
        case semantic::TypeKind::FLOAT:  return 4;
        case semantic::TypeKind::BOOL:   return 1;
        case semantic::TypeKind::VOID:   return 0;
        case semantic::TypeKind::STRING: return 8;
        case semantic::TypeKind::STRUCT: {
            auto info = type->getStructInfo();
            if (!info) return 8;
            int total = 0;
            for (const auto& field : info->fields) {
                total += getTypeSize(field.second);
            }
            return total;
        }
        default: return 8;
    }
}

int ABI::getTypeAlignment(semantic::Type* type) {
    if (!type) return 4;
    
    switch (type->getKind()) {
        case semantic::TypeKind::INT:    return 4;
        case semantic::TypeKind::FLOAT:  return 4;
        case semantic::TypeKind::BOOL:   return 1;
        case semantic::TypeKind::STRING: return 8;
        default: return 8;
    }
}

std::string ABI::getRegName(const std::string& reg64, int size) {
    if (size == 1) {
        if (reg64 == "rax") return "al";
        if (reg64 == "rbx") return "bl";
        if (reg64 == "rcx") return "cl";
        if (reg64 == "rdx") return "dl";
        if (reg64 == "rsi") return "sil";
        if (reg64 == "rdi") return "dil";
        if (reg64 == "rbp") return "bpl";
        if (reg64 == "rsp") return "spl";
        if (reg64 == "r8")  return "r8b";
        if (reg64 == "r9")  return "r9b";
        if (reg64 == "r10") return "r10b";
        if (reg64 == "r11") return "r11b";
        if (reg64 == "r12") return "r12b";
        if (reg64 == "r13") return "r13b";
        if (reg64 == "r14") return "r14b";
        if (reg64 == "r15") return "r15b";
        return reg64;
    } else if (size == 4) {
        std::string r = reg64;
        if (r.length() >= 2 && r[0] == 'r') {
            r[0] = 'e';
            return r;
        }
        if (r == "r8")  return "r8d";
        if (r == "r9")  return "r9d";
        if (r == "r10") return "r10d";
        if (r == "r11") return "r11d";
        if (r == "r12") return "r12d";
        if (r == "r13") return "r13d";
        if (r == "r14") return "r14d";
        if (r == "r15") return "r15d";
        return reg64;
    }
    return reg64;
}

bool ABI::isArgInRegister(int index, bool is_float) {
    if (is_float) {
        return index < static_cast<int>(ARG_REGISTERS_FLOAT.size());
    }
    return index < static_cast<int>(ARG_REGISTERS_INT.size());
}

std::string ABI::getArgRegister(int index, bool is_float) {
    if (is_float) {
        if (index >= 0 && index < static_cast<int>(ARG_REGISTERS_FLOAT.size())) {
            return ARG_REGISTERS_FLOAT[index];
        }
    } else {
        if (index >= 0 && index < static_cast<int>(ARG_REGISTERS_INT.size())) {
            return ARG_REGISTERS_INT[index];
        }
    }
    return "";
}

int ABI::getStackArgOffset(int index, bool is_float) {
    int reg_count = is_float ? static_cast<int>(ARG_REGISTERS_FLOAT.size()) 
                             : static_cast<int>(ARG_REGISTERS_INT.size());
    
    int stack_index = index - reg_count;
    if (stack_index >= 0) {
        return 16 + (stack_index * 8);
    }
    return -1;
}

RegisterAllocator::RegisterAllocator() 
    : next_spill_offset(0)
    , max_spill_offset(0)
    , allocation_done(false)
    , total_vars(0)
    , vars_in_regs(0)
    , vars_spilled(0) {
    
    gp_regs_32 = {"eax", "ecx", "edx", "esi", "edi", "r8d", "r9d", "r10d", "r11d"};
    gp_regs_64 = {"rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"};
    
    reset();
}

void RegisterAllocator::reset() {
    reg_states.clear();
    live_ranges.clear();
    var_to_reg.clear();
    var_to_mem.clear();
    
    for (const auto& reg : gp_regs_32) {
        reg_states[reg] = RegState();
    }
    
    next_spill_offset = 0;
    max_spill_offset = 0;
    allocation_done = false;
    total_vars = 0;
    vars_in_regs = 0;
    vars_spilled = 0;
}

std::string RegisterAllocator::extractBaseName(const std::string& ssa_name) {
    size_t pos = ssa_name.rfind('_');
    if (pos != std::string::npos && pos > 0 && pos < ssa_name.length() - 1) {
        bool is_version = true;
        for (size_t i = pos + 1; i < ssa_name.length(); ++i) {
            if (!std::isdigit(ssa_name[i])) {
                is_version = false;
                break;
            }
        }
        if (is_version) {
            return ssa_name.substr(0, pos);
        }
    }
    return ssa_name;
}

void RegisterAllocator::addVarUse(const std::string& var_name, int instr_index) {
    if (var_name.empty()) return;
    
    auto it = std::find_if(live_ranges.begin(), live_ranges.end(),
        [&var_name](const LiveRange& lr) {
            return lr.var_name == var_name;
        });
    
    if (it != live_ranges.end()) {
        it->start = std::min(it->start, instr_index);
        it->end   = std::max(it->end,   instr_index);
    } else {
        LiveRange lr;
        lr.var_name   = var_name;
        lr.start      = instr_index;
        lr.end        = instr_index;
        lr.is_spilled = false;
        lr.spill_slot = 0;
        live_ranges.push_back(lr);
    }
}

void RegisterAllocator::collectLiveRanges(const ir::IRFunction* func) {
    live_ranges.clear();
    var_to_reg.clear();
    var_to_mem.clear();
    
    int global_idx = 0;
    
    for (const auto& block : func->getBlocks()) {
        for (const auto& instr : block->getInstructions()) {
            if (instr->dest.kind == ir::OperandKind::TEMP || 
                instr->dest.kind == ir::OperandKind::VARIABLE) {
                addVarUse(instr->dest.name, global_idx);
            }
            
            if (instr->src1.kind == ir::OperandKind::TEMP || 
                instr->src1.kind == ir::OperandKind::VARIABLE) {
                addVarUse(instr->src1.name, global_idx);
            }
            
            if (instr->src2.kind == ir::OperandKind::TEMP || 
                instr->src2.kind == ir::OperandKind::VARIABLE) {
                addVarUse(instr->src2.name, global_idx);
            }
            
            for (const auto& arg : instr->args) {
                if (arg.kind == ir::OperandKind::TEMP || 
                    arg.kind == ir::OperandKind::VARIABLE) {
                    addVarUse(arg.name, global_idx);
                }
            }
            
            global_idx++;
        }
    }
    
    std::sort(live_ranges.begin(), live_ranges.end());
    total_vars = static_cast<int>(live_ranges.size());
}

void RegisterAllocator::expireOldIntervals(int current_pos,
                                           std::vector<ActiveInterval>& active) {
    std::vector<ActiveInterval> still_active;
    
    for (const auto& interval : active) {
        if (interval.end < current_pos) {
            reg_states[interval.reg].in_use   = false;
            reg_states[interval.reg].var_name = "";
        } else {
            still_active.push_back(interval);
        }
    }
    
    active = std::move(still_active);
}

void RegisterAllocator::allocateRegisters(const ir::IRFunction* func) {
    if (live_ranges.empty()) {
        collectLiveRanges(func);
    }
    
    var_to_reg.clear();
    var_to_mem.clear();
    next_spill_offset = 0;
    max_spill_offset  = 0;
    vars_in_regs      = 0;
    vars_spilled      = 0;
    
    for (auto& pair : reg_states) {
        pair.second.in_use   = false;
        pair.second.var_name = "";
    }
    
    std::unordered_map<std::string, std::vector<LiveRange*>> base_groups;
    for (auto& lr : live_ranges) {
        std::string base = extractBaseName(lr.var_name);
        base_groups[base].push_back(&lr);
    }
    
    for (auto& [base, ranges] : base_groups) {
        if (ranges.size() <= 1) continue;
        
        int slot = -(8 + next_spill_offset);
        next_spill_offset += 4;
        next_spill_offset = (next_spill_offset + 3) & ~3;
        max_spill_offset = std::max(max_spill_offset, next_spill_offset);
        
        std::string mem_ref = "dword [rbp" + std::to_string(slot) + "]";
        for (auto* lr : ranges) {
            lr->is_spilled = true;
            lr->spill_slot = slot;
            var_to_mem[lr->var_name] = mem_ref;
        }
        vars_spilled += static_cast<int>(ranges.size());
    }
    
    std::vector<ActiveInterval> active;
    
    for (auto& lr : live_ranges) {
        if (lr.is_spilled) continue;
        
        expireOldIntervals(lr.start, active);
        
        std::string free_reg = "";
        for (const auto& reg : gp_regs_32) {
            if (!reg_states[reg].in_use) {
                free_reg = reg;
                break;
            }
        }
        
        if (!free_reg.empty()) {
            reg_states[free_reg].in_use   = true;
            reg_states[free_reg].var_name = lr.var_name;
            var_to_reg[lr.var_name]       = free_reg;
            
            ActiveInterval ai;
            ai.var_name = lr.var_name;
            ai.end      = lr.end;
            ai.reg      = free_reg;
            active.push_back(ai);
            
            vars_in_regs++;
        } else {
            int slot = -(8 + next_spill_offset);
            next_spill_offset += 4;
            next_spill_offset = (next_spill_offset + 3) & ~3;
            max_spill_offset = std::max(max_spill_offset, next_spill_offset);
            
            lr.is_spilled = true;
            lr.spill_slot = slot;
            var_to_mem[lr.var_name] = "dword [rbp" + std::to_string(slot) + "]";
            vars_spilled++;
        }
    }
    
    allocation_done = true;
}

std::string RegisterAllocator::getReg(const std::string& var_name) const {
    auto it = var_to_reg.find(var_name);
    return (it != var_to_reg.end()) ? it->second : "";
}

std::string RegisterAllocator::getMem(const std::string& var_name) const {
    auto it = var_to_mem.find(var_name);
    return (it != var_to_mem.end()) ? it->second : "";
}

bool RegisterAllocator::isInRegister(const std::string& var_name) const {
    return var_to_reg.find(var_name) != var_to_reg.end();
}

bool RegisterAllocator::isSpilled(const std::string& var_name) const {
    return var_to_mem.find(var_name) != var_to_mem.end();
}

int RegisterAllocator::getSpillSlotOffset(const std::string& var_name) const {
    for (const auto& lr : live_ranges) {
        if (lr.var_name == var_name && lr.is_spilled) {
            return lr.spill_slot;
        }
    }
    return 0;
}

int RegisterAllocator::getTotalSpillSize() const {
    return (max_spill_offset + 15) & ~15;
}

void RegisterAllocator::printStats() const {
    std::cerr << "\n";
    std::cerr << "[Register Allocation Statistics]\n";
    std::cerr << "Total variables:       " << total_vars << "\n";
    std::cerr << "In registers:          " << vars_in_regs << "\n";
    std::cerr << "Spilled to memory:     " << vars_spilled << "\n";
    std::cerr << "Available registers:   " << gp_regs_32.size() << "\n";
    
    if (total_vars > 0) {
        int usage_pct = (vars_in_regs * 100) / total_vars;
        std::cerr << "Register usage rate:   " << usage_pct << "%\n";
    }
    
    std::cerr << "Total spill size:      " << getTotalSpillSize() << " bytes\n";
}

}