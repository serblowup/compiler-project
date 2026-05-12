#include "register_allocator.hpp"
#include "../ir/ir.hpp"
#include <algorithm>
#include <iostream>
#include <cctype>
#include <unordered_set>

namespace codegen {

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

void RegisterAllocator::spillFarthestInterval(
    int current_pos,
    std::vector<ActiveInterval>& active,
    LiveRange& current_lr) {
    
    (void)current_pos;
    
    int farthest_idx = -1;
    int farthest_end = -1;
    
    for (size_t i = 0; i < active.size(); ++i) {
        if (active[i].end > farthest_end) {
            farthest_end = active[i].end;
            farthest_idx = static_cast<int>(i);
        }
    }
    
    if (farthest_idx == -1) {
        int slot = -(8 + next_spill_offset);
        next_spill_offset += 4;
        next_spill_offset = (next_spill_offset + 3) & ~3;
        max_spill_offset = std::max(max_spill_offset, next_spill_offset);
        
        current_lr.is_spilled = true;
        current_lr.spill_slot = slot;
        var_to_mem[current_lr.var_name] = "dword [rbp" + std::to_string(slot) + "]";
        vars_spilled++;
        return;
    }
    
    std::string spill_var = active[farthest_idx].var_name;
    std::string spill_reg = active[farthest_idx].reg;
    
    int slot = -(8 + next_spill_offset);
    next_spill_offset += 4;
    next_spill_offset = (next_spill_offset + 3) & ~3;
    max_spill_offset = std::max(max_spill_offset, next_spill_offset);
    
    for (auto& lr : live_ranges) {
        if (lr.var_name == spill_var) {
            lr.is_spilled = true;
            lr.spill_slot = slot;
            break;
        }
    }
    var_to_mem[spill_var] = "dword [rbp" + std::to_string(slot) + "]";
    var_to_reg.erase(spill_var);
    vars_spilled++;
    vars_in_regs--;
    
    active.erase(active.begin() + farthest_idx);
    
    reg_states[spill_reg].in_use   = true;
    reg_states[spill_reg].var_name = current_lr.var_name;
    var_to_reg[current_lr.var_name] = spill_reg;
    
    ActiveInterval ai;
    ai.var_name = current_lr.var_name;
    ai.end      = current_lr.end;
    ai.reg      = spill_reg;
    active.push_back(ai);
    
    vars_in_regs++;
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
    
    bool has_calls = false;
    for (const auto& block : func->getBlocks()) {
        for (const auto& instr : block->getInstructions()) {
            if (instr->kind == ir::InstrKind::CALL) {
                has_calls = true;
                break;
            }
        }
        if (has_calls) break;
    }
    
    std::unordered_map<std::string, std::vector<LiveRange*>> base_groups;
    for (auto& lr : live_ranges) {
        std::string base = extractBaseName(lr.var_name);
        base_groups[base].push_back(&lr);
    }
    
    std::unordered_set<std::string> pre_spilled_vars;
    
    if (has_calls) {
        for (auto& [base, ranges] : base_groups) {
            bool is_user_var = false;
            for (auto* lr : ranges) {
                std::string name = lr->var_name;
                if (name[0] != 't' && name.find('_') != std::string::npos) {
                    is_user_var = true;
                    break;
                }
            }
            
            if (is_user_var) {
                int slot = -(8 + next_spill_offset);
                next_spill_offset += 4;
                next_spill_offset = (next_spill_offset + 3) & ~3;
                max_spill_offset = std::max(max_spill_offset, next_spill_offset);
                
                std::string mem_ref = "dword [rbp" + std::to_string(slot) + "]";
                
                for (auto* lr : ranges) {
                    lr->is_spilled = true;
                    lr->spill_slot = slot;
                    var_to_mem[lr->var_name] = mem_ref;
                    pre_spilled_vars.insert(lr->var_name);
                    vars_spilled++;
                }
            }
        }
    }
    
    for (auto& [base, ranges] : base_groups) {
        if (ranges.size() <= 1) continue;
        if (pre_spilled_vars.count(ranges[0]->var_name) > 0) continue;
        
        int slot = -(8 + next_spill_offset);
        next_spill_offset += 4;
        next_spill_offset = (next_spill_offset + 3) & ~3;
        max_spill_offset = std::max(max_spill_offset, next_spill_offset);
        
        std::string mem_ref = "dword [rbp" + std::to_string(slot) + "]";
        
        for (auto* lr : ranges) {
            lr->is_spilled = true;
            lr->spill_slot = slot;
            var_to_mem[lr->var_name] = mem_ref;
            pre_spilled_vars.insert(lr->var_name);
            vars_spilled++;
        }
    }
    
    std::vector<ActiveInterval> active;
    
    for (auto& lr : live_ranges) {
        if (pre_spilled_vars.count(lr.var_name) > 0) {
            continue;
        }
        
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
            spillFarthestInterval(lr.start, active, lr);
        }
    }
    
    allocation_done = true;
}

std::string RegisterAllocator::getReg(const std::string& var_name) const {
    auto it = var_to_reg.find(var_name);
    if (it != var_to_reg.end()) {
        return it->second;
    }
    std::string base = extractBaseName(var_name);
    if (base != var_name) {
        it = var_to_reg.find(base);
        if (it != var_to_reg.end()) {
            return it->second;
        }
    }
    return "";
}

std::string RegisterAllocator::getMem(const std::string& var_name) const {
    auto it = var_to_mem.find(var_name);
    if (it != var_to_mem.end()) {
        return it->second;
    }
    std::string base = extractBaseName(var_name);
    if (base != var_name) {
        it = var_to_mem.find(base);
        if (it != var_to_mem.end()) {
            return it->second;
        }
    }
    return "";
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
    std::string base = extractBaseName(var_name);
    if (base != var_name) {
        for (const auto& lr : live_ranges) {
            std::string lr_base = extractBaseName(lr.var_name);
            if (lr_base == base && lr.is_spilled) {
                return lr.spill_slot;
            }
        }
    }
    return 0;
}

int RegisterAllocator::getTotalSpillSize() const {
    if (max_spill_offset == 0) return 0;
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