#ifndef CODEGEN_PEEPHOLE_OPTIMIZER_HPP
#define CODEGEN_PEEPHOLE_OPTIMIZER_HPP

#include <string>
#include <vector>
#include <sstream>
#include <regex>

namespace codegen {

class PeepholeOptimizer {
public:
    struct OptimizationStats {
        int redundant_moves_removed = 0;
        int redundant_loads_removed = 0;
        int inc_dec_replacements = 0;
        int xor_zero_replacements = 0;
        int test_removed = 0;
        int total_removed = 0;
        
        std::string toString() const;
    };
    
    PeepholeOptimizer();
    
    std::string optimize(const std::string& asm_code);
    
    const OptimizationStats& getStats() const;
    
    void resetStats();
    
private:
    OptimizationStats stats;
    
    struct AsmLine {
        std::string label;
        std::string instruction;
        std::string operands;
        std::string comment;
        std::string raw;
        bool is_label;
        bool is_directive;
        bool has_indent;
        
        AsmLine() : is_label(false), is_directive(false), has_indent(false) {}
    };
    
    AsmLine parseLine(const std::string& line);
    
    bool applyRedundantMoveElimination(std::vector<AsmLine>& lines);
    bool applyStoreLoadElimination(std::vector<AsmLine>& lines);
    bool applyIncDecReplacement(std::vector<AsmLine>& lines);
    bool applyXorZeroReplacement(std::vector<AsmLine>& lines);
    bool applyTestBeforeJumpElimination(std::vector<AsmLine>& lines);
    
    std::string rebuildCode(const std::vector<AsmLine>& lines);
};

}

#endif