#ifndef CODEGEN_EXPRESSION_GENERATOR_HPP
#define CODEGEN_EXPRESSION_GENERATOR_HPP

#include <string>
#include <vector>
#include "label_manager.hpp"

namespace ir {
    class Instruction;
}

namespace codegen {

class ExpressionGenerator {
private:
    LabelManager& label_manager;
    std::string indent;
    
public:
    explicit ExpressionGenerator(LabelManager& lm);
    
    std::string label(const std::string& label_name);
    
    std::vector<std::string> generateArithmetic(const ir::Instruction* instr);
    std::vector<std::string> generateAdd(const ir::Instruction* instr);
    std::vector<std::string> generateSub(const ir::Instruction* instr);
    std::vector<std::string> generateMul(const ir::Instruction* instr);
    std::vector<std::string> generateDiv(const ir::Instruction* instr);
    std::vector<std::string> generateMod(const ir::Instruction* instr);
    
    std::vector<std::string> generateComparison(const ir::Instruction* instr);
    
    std::vector<std::string> generateShortCircuitAnd(const ir::Instruction* instr);
    std::vector<std::string> generateShortCircuitOr(const ir::Instruction* instr);
    std::vector<std::string> generateNot(const ir::Instruction* instr);
    
    void setIndent(const std::string& ind) { indent = ind; }
};

}

#endif