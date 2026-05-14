#include "control_flow_generator.hpp"
#include "../ir/ir.hpp"
#include <sstream>
#include <cctype>

namespace codegen {

ControlFlowGenerator::ControlFlowGenerator(LabelManager& lm) 
    : label_manager(lm), indent("    ") {
}

bool ControlFlowGenerator::isRegister(const std::string& operand) {
    if (operand.empty()) return false;
    char first = operand[0];
    if (first == 'r' || first == 'e' || first == 'a' || first == 'b' ||
        first == 'c' || first == 'd' || first == 's' || first == 'd' ||
        first == 'x' || first == 'y' || first == 'z') {
        return true;
    }
    return false;
}

std::string ControlFlowGenerator::jump(const std::string& target) {
    return indent + "jmp " + target;
}

std::string ControlFlowGenerator::jumpIf(const std::string& cond_reg, const std::string& target) {
    std::ostringstream oss;
    if (isRegister(cond_reg) || cond_reg.find('[') != std::string::npos) {
        oss << indent << "test " << cond_reg << ", " << cond_reg << "\n";
        oss << indent << "jnz " << target;
    } else {
        oss << indent << "mov eax, " << cond_reg << "\n";
        oss << indent << "test eax, eax\n";
        oss << indent << "jnz " << target;
    }
    return oss.str();
}

std::string ControlFlowGenerator::jumpIfNot(const std::string& cond_reg, const std::string& target) {
    std::ostringstream oss;
    if (isRegister(cond_reg) || cond_reg.find('[') != std::string::npos) {
        oss << indent << "test " << cond_reg << ", " << cond_reg << "\n";
        oss << indent << "jz " << target;
    } else {
        oss << indent << "mov eax, " << cond_reg << "\n";
        oss << indent << "test eax, eax\n";
        oss << indent << "jz " << target;
    }
    return oss.str();
}

std::string ControlFlowGenerator::label(const std::string& label_name) {
    return label_name + ":\n";
}

std::vector<std::string> ControlFlowGenerator::generateIf(const ir::Instruction* instr) {
    std::vector<std::string> result;
    
    std::string else_label = label_manager.newLabel("else");
    std::string end_label = label_manager.newLabel("endif");
    
    std::string cond_reg = getCondOp(instr);
    if (!cond_reg.empty()) {
        result.push_back(jumpIfNot(cond_reg, else_label));
    }
    
    result.push_back(jump(end_label));
    result.push_back(label(else_label));
     
    result.push_back(label(end_label));
    
    return result;
}

std::string ControlFlowGenerator::getCondOp(const ir::Instruction* instr) {
    if (instr->src1.kind == ir::OperandKind::TEMP) {
        return instr->src1.name;
    }
    return "";
}

}