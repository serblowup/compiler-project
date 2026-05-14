#ifndef CODEGEN_CONTROL_FLOW_GENERATOR_HPP
#define CODEGEN_CONTROL_FLOW_GENERATOR_HPP

#include <string>
#include <vector>
#include <memory>
#include "label_manager.hpp"

namespace ir {
    class Instruction;
    class BasicBlock;
}

namespace codegen {

class ControlFlowGenerator {
private:
    LabelManager& label_manager;
    std::string indent;
    
    std::string getCondOp(const ir::Instruction* instr);
    bool isRegister(const std::string& operand);
    
public:
    explicit ControlFlowGenerator(LabelManager& lm);
    
    std::string jump(const std::string& target);
    std::string jumpIf(const std::string& cond_reg, const std::string& target);
    std::string jumpIfNot(const std::string& cond_reg, const std::string& target);
    std::string label(const std::string& label_name);
    
    std::vector<std::string> generateIf(const ir::Instruction* instr);
    std::vector<std::string> generateIfElse(const ir::Instruction* instr);
    std::vector<std::string> generateWhile(const ir::Instruction* instr);
    std::vector<std::string> generateFor(const ir::Instruction* instr);
    
    std::string generateBreak();
    std::string generateContinue();
    
    void setIndent(const std::string& ind) { indent = ind; }
};

}

#endif