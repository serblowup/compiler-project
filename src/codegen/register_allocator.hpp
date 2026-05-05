#ifndef CODEGEN_REGISTER_ALLOCATOR_HPP
#define CODEGEN_REGISTER_ALLOCATOR_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>

namespace ir {
    class IRFunction;
}

namespace codegen {

class RegisterAllocator {
public:
    struct LiveRange {
        std::string var_name;
        int start;
        int end;
        bool is_spilled;
        int spill_slot;
        
        LiveRange() : start(0), end(0), is_spilled(false), spill_slot(0) {}
        
        bool operator<(const LiveRange& other) const {
            if (start != other.start) return start < other.start;
            return end < other.end;
        }
    };
    
    struct ActiveInterval {
        std::string var_name;
        int end;
        std::string reg;
        
        bool operator<(const ActiveInterval& other) const {
            return end < other.end;
        }
    };

private:
    std::vector<std::string> gp_regs_32;
    std::vector<std::string> gp_regs_64;
    
    struct RegState {
        bool in_use;
        std::string var_name;
        
        RegState() : in_use(false), var_name("") {}
    };
    std::unordered_map<std::string, RegState> reg_states;
    
    std::vector<LiveRange> live_ranges;
    
    std::unordered_map<std::string, std::string> var_to_reg;
    std::unordered_map<std::string, std::string> var_to_mem;
    
    int next_spill_offset;
    int max_spill_offset;
    
    bool allocation_done;
    
    int total_vars;
    int vars_in_regs;
    int vars_spilled;

public:
    RegisterAllocator();
    
    void collectLiveRanges(const ir::IRFunction* func);
    void allocateRegisters(const ir::IRFunction* func);
    
    std::string getReg(const std::string& var_name) const;
    std::string getMem(const std::string& var_name) const;
    bool isInRegister(const std::string& var_name) const;
    bool isSpilled(const std::string& var_name) const;
    
    int getSpillSlotOffset(const std::string& var_name) const;
    int getTotalSpillSize() const;

    void printStats() const;
    void reset();
    
private:
    void expireOldIntervals(int current_pos, 
                           std::vector<ActiveInterval>& active);
    void addVarUse(const std::string& var_name, int instr_index);
    static std::string extractBaseName(const std::string& ssa_name);
};

}

#endif