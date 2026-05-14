#include "expression_generator.hpp"
#include "../ir/ir.hpp"
#include <sstream>

namespace codegen {

ExpressionGenerator::ExpressionGenerator(LabelManager& lm) 
    : label_manager(lm), indent("    ") {
}

std::string ExpressionGenerator::label(const std::string& label_name) {
    return label_name + ":";
}

std::vector<std::string> ExpressionGenerator::generateShortCircuitAnd(const ir::Instruction* instr) {
    std::vector<std::string> result;
    std::string false_label = label_manager.newLabel("false");
    std::string end_label = label_manager.newLabel("end");
    
    result.push_back(indent + "mov eax, " + instr->src1.name);
    result.push_back(indent + "test eax, eax");
    result.push_back(indent + "jz " + false_label);
    
    result.push_back(indent + "mov eax, " + instr->src2.name);
    result.push_back(indent + "test eax, eax");
    result.push_back(indent + "jz " + false_label);
    
    result.push_back(indent + "mov eax, 1");
    result.push_back(indent + "jmp " + end_label);
    
    result.push_back("");
    result.push_back(label(false_label));
    result.push_back(indent + "mov eax, 0");
    
    result.push_back("");
    result.push_back(label(end_label));
    result.push_back(indent + "mov " + instr->dest.name + ", eax");
    
    return result;
}

std::vector<std::string> ExpressionGenerator::generateShortCircuitOr(const ir::Instruction* instr) {
    std::vector<std::string> result;
    std::string true_label = label_manager.newLabel("true");
    std::string end_label = label_manager.newLabel("end");
    
    result.push_back(indent + "mov eax, " + instr->src1.name);
    result.push_back(indent + "test eax, eax");
    result.push_back(indent + "jnz " + true_label);
    
    result.push_back(indent + "mov eax, " + instr->src2.name);
    result.push_back(indent + "test eax, eax");
    result.push_back(indent + "jnz " + true_label);
    
    result.push_back(indent + "mov eax, 0");
    result.push_back(indent + "jmp " + end_label);
    
    result.push_back("");
    result.push_back(label(true_label));
    result.push_back(indent + "mov eax, 1");
    
    result.push_back("");
    result.push_back(label(end_label));
    result.push_back(indent + "mov " + instr->dest.name + ", eax");
    
    return result;
}

std::vector<std::string> ExpressionGenerator::generateNot(const ir::Instruction* instr) {
    std::vector<std::string> result;
    result.push_back(indent + "mov eax, " + instr->src1.name);
    result.push_back(indent + "xor eax, 1");
    result.push_back(indent + "mov " + instr->dest.name + ", eax");
    return result;
}

}