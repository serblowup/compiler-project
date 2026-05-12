#include "x86_generator.hpp"
#include "peephole_optimizer.hpp"
#include <algorithm>
#include <stdexcept>
#include <cctype>
#include <set>

namespace codegen {

X86Generator::X86Generator() 
    : ir_program(nullptr)
    , current_function(nullptr)
    , current_frame(nullptr)
    , label_counter(0)
    , string_counter(0)
    , current_block_ended(false)
    , use_register_allocation(true)
    , use_peephole_optimization(true)
    , is_leaf_function(false)
    , last_call_dest("")
{
}

void X86Generator::emit(const std::string& asm_code) {
    if (!asm_code.empty()) {
        out << "    " << asm_code << "\n";
    }
}

void X86Generator::emitLabel(const std::string& label) {
    out << label << ":\n";
}

void X86Generator::emitComment(const std::string& comment) {
    out << "    ; " << comment << "\n";
}

void X86Generator::emitDirective(const std::string& directive) {
    out << directive << "\n";
}

std::string X86Generator::extractBaseName(const std::string& ssa_name) {
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

std::string X86Generator::getOperand(const ir::Operand& op) {
    switch (op.kind) {
        case ir::OperandKind::TEMP:
        case ir::OperandKind::VARIABLE: {
            auto it = var_mapping.find(op.name);
            if (it != var_mapping.end()) {
                return it->second;
            }
            
            std::string base = extractBaseName(op.name);
            if (base != op.name) {
                it = var_mapping.find(base);
                if (it != var_mapping.end()) {
                    std::string loc = it->second;
                    var_mapping[op.name] = loc;
                    return loc;
                }
            }
            
            if (use_register_allocation) {
                std::string reg = reg_alloc.getReg(op.name);
                if (!reg.empty()) {
                    var_mapping[op.name] = reg;
                    return reg;
                }
                
                if (base != op.name) {
                    reg = reg_alloc.getReg(base);
                    if (!reg.empty()) {
                        var_mapping[op.name] = reg;
                        return reg;
                    }
                }
                
                std::string mem = reg_alloc.getMem(op.name);
                if (!mem.empty()) {
                    var_mapping[op.name] = mem;
                    return mem;
                }
                
                if (base != op.name) {
                    mem = reg_alloc.getMem(base);
                    if (!mem.empty()) {
                        var_mapping[op.name] = mem;
                        return mem;
                    }
                }
            }
            
            return fallbackGetOperand(op);
        }
        
        case ir::OperandKind::CONST_INT:
            return std::to_string(op.int_value);
            
        case ir::OperandKind::CONST_FLOAT:
            return std::to_string(op.float_value);
            
        case ir::OperandKind::CONST_BOOL:
            return op.bool_value ? "1" : "0";
            
        case ir::OperandKind::LABEL:
            return op.name;
            
        case ir::OperandKind::MEMORY:
            if (op.mem_offset == 0)
                return "[" + op.mem_base + "]";
            else if (op.mem_offset > 0)
                return "[" + op.mem_base + "+" + std::to_string(op.mem_offset) + "]";
            else
                return "[" + op.mem_base + std::to_string(op.mem_offset) + "]";
            
        default:
            return "0";
    }
}

std::string X86Generator::fallbackGetOperand(const ir::Operand& op) {
    std::string base = extractBaseName(op.name);
    
    if (current_frame && current_frame->hasVar(base)) {
        int offset = current_frame->getVarOffset(base);
        std::string mem = "dword [rbp" + std::to_string(offset) + "]";
        var_mapping[op.name] = mem;
        var_mapping[base] = mem;
        return mem;
    }
    
    if (current_function) {
        for (const auto& param : current_function->getParameters()) {
            if (param.name == base || param.name == op.name) {
                if (current_frame && current_frame->hasVar(param.name)) {
                    int offset = current_frame->getVarOffset(param.name);
                    std::string mem = "dword [rbp" + std::to_string(offset) + "]";
                    var_mapping[op.name] = mem;
                    var_mapping[base] = mem;
                    return mem;
                }
            }
        }
    }
    
    int spill_offset = current_frame->allocateSpillSlot(op.name, 4);
    std::string mem = "dword [rbp" + std::to_string(spill_offset) + "]";
    var_mapping[op.name] = mem;
    var_mapping[base] = mem;
    
    return mem;
}

std::string X86Generator::getDestOperand(const ir::Operand& dest, const ir::Operand& src) {
    (void)src;
    return getOperand(dest);
}

void X86Generator::loadToReg(const std::string& reg, const ir::Operand& src) {
    std::string src_op = getOperand(src);
    
    if (src_op == reg) {
        return;
    }
    
    if (src.kind == ir::OperandKind::CONST_INT ||
        src.kind == ir::OperandKind::CONST_BOOL ||
        src.kind == ir::OperandKind::CONST_FLOAT) {
        emit("mov " + reg + ", " + src_op);
    } else if (src_op.find("[rbp") != std::string::npos || 
               src_op[0] == '[') {
        emit("mov " + reg + ", " + src_op);
    } else {
        emit("mov " + reg + ", " + src_op);
    }
}

void X86Generator::storeFromReg(const std::string& reg, const ir::Operand& dest) {
    std::string dest_op = getOperand(dest);
    
    if (dest_op == reg) {
        return;
    }
    
    if (dest_op.find("[rbp") != std::string::npos || 
        dest_op[0] == '[') {
        emit("mov " + dest_op + ", " + reg);
    } else {
        emit("mov " + dest_op + ", " + reg);
    }
}

bool X86Generator::isLeafFunction(const ir::IRFunction* func) const {
    for (const auto& block : func->getBlocks()) {
        for (const auto& instr : block->getInstructions()) {
            if (instr->kind == ir::InstrKind::CALL) {
                return false;
            }
        }
    }
    return true;
}

void X86Generator::generatePrologue(const ir::IRFunction* func) {
    emitComment("Function prologue: " + func->getName());
    
    emit("push rbp");
    emit("mov rbp, rsp");
    
    int stack_size = current_frame ? current_frame->getTotalStackSize() : 0;
    int total_frame_size = stack_size;
    
    if (current_frame) {
        total_frame_size += current_frame->getSavedRegsSize();
    }
    
    const auto& saved_regs = current_frame ? current_frame->getSavedRegisters() : std::vector<std::string>();
    bool use_red_zone = is_leaf_function && total_frame_size <= ABI::RED_ZONE_SIZE && saved_regs.empty();
    
    if (use_red_zone) {
        emitComment("Leaf function - using red zone");
    } else if (stack_size > 0) {
        emit("sub rsp, " + std::to_string(stack_size));
        emitComment("Allocated " + std::to_string(stack_size) + " bytes for locals");
    }
    
    if (current_frame) {
        for (const auto& reg : saved_regs) {
            if (reg != "rbp" && reg != "rsp") {
                emit("push " + reg);
            }
        }
    }
    
    int arg_index = 0;
    for (const auto& param : func->getParameters()) {
        if (current_frame && current_frame->hasVar(param.name)) {
            int offset = current_frame->getVarOffset(param.name);
            if (arg_index < 6) {
                std::string reg = ABI::getArgRegister(arg_index, false);
                std::string mem = "dword [rbp" + std::to_string(offset) + "]";
                
                emit("mov " + mem + ", " + ABI::getRegName(reg, 4));
                
                var_mapping[param.name] = mem;
                
                for (const auto& block : func->getBlocks()) {
                    for (const auto& instr : block->getInstructions()) {
                        if (instr->kind == ir::InstrKind::PARAM) {
                            std::string ssa_name = instr->src2.name;
                            std::string base = extractBaseName(ssa_name);
                            if (base == param.name) {
                                var_mapping[ssa_name] = mem;
                            }
                        }
                    }
                }
            }
        }
        arg_index++;
    }
    
    emit("");
}

void X86Generator::generateEpilogue(const ir::IRFunction* func) {
    emitComment("Function epilogue: " + func->getName());
    
    if (current_frame) {
        const auto& saved_regs = current_frame->getSavedRegisters();
        for (auto it = saved_regs.rbegin(); it != saved_regs.rend(); ++it) {
            if (*it != "rbp" && *it != "rsp") {
                emit("pop " + *it);
            }
        }
    }
    
    if (!current_frame || current_frame->getTotalStackSize() > 0) {
        emit("mov rsp, rbp");
    }
    emit("pop rbp");
    emit("ret");
    emit("");
}

void X86Generator::generateInstruction(const ir::Instruction* instr) {
    if (current_block_ended) {
        return;
    }
    
    if (instr->kind != ir::InstrKind::CALL) {
        last_call_dest = "";
    }
    
    switch (instr->kind) {
        case ir::InstrKind::ADD:
        case ir::InstrKind::SUB:
        case ir::InstrKind::MUL:
        case ir::InstrKind::DIV:
        case ir::InstrKind::MOD:
        case ir::InstrKind::NEG:
            generateArithmetic(instr);
            break;
            
        case ir::InstrKind::CMP_EQ:
        case ir::InstrKind::CMP_NE:
        case ir::InstrKind::CMP_LT:
        case ir::InstrKind::CMP_LE:
        case ir::InstrKind::CMP_GT:
        case ir::InstrKind::CMP_GE:
            generateComparison(instr);
            break;
            
        case ir::InstrKind::AND:
        case ir::InstrKind::OR:
        case ir::InstrKind::NOT:
            generateLogic(instr);
            break;
            
        case ir::InstrKind::LOAD:
        case ir::InstrKind::STORE:
        case ir::InstrKind::ALLOCA:
            generateMemory(instr);
            break;
            
        case ir::InstrKind::JUMP:
        case ir::InstrKind::JUMP_IF:
        case ir::InstrKind::JUMP_IF_NOT:
        case ir::InstrKind::LABEL:
            generateControlFlow(instr);
            break;
            
        case ir::InstrKind::CALL:
            generateCall(instr);
            break;
            
        case ir::InstrKind::RETURN:
            generateReturn(instr);
            break;
            
        case ir::InstrKind::PHI:
            generatePhi(instr);
            break;
            
        case ir::InstrKind::MOVE:
            generateMove(instr);
            break;
            
        case ir::InstrKind::PARAM:
            break;
            
        default:
            emitComment("Unknown instruction: " + 
                       std::string(ir::instrKindToString(instr->kind)));
            break;
    }
}

void X86Generator::generateArithmetic(const ir::Instruction* instr) {
    std::string src1_op = getOperand(instr->src1);
    std::string src2_op = getOperand(instr->src2);
    
    if (instr->kind == ir::InstrKind::DIV || instr->kind == ir::InstrKind::MOD) {
        loadToReg("eax", instr->src1);
        emit("cdq");
        emit("idiv " + src2_op);
        
        if (instr->kind == ir::InstrKind::DIV) {
            storeFromReg("eax", instr->dest);
        } else {
            storeFromReg("edx", instr->dest);
        }
        return;
    }
    
    std::string dest_op = getOperand(instr->dest);
    bool dest_is_mem = (dest_op.find("[rbp") != std::string::npos || 
                        dest_op[0] == '[');
    
    std::string work_reg;
    if (!dest_is_mem && instr->kind != ir::InstrKind::NEG) {
        work_reg = dest_op;
        loadToReg(work_reg, instr->src1);
    } else {
        work_reg = "eax";
        loadToReg(work_reg, instr->src1);
    }
    
    switch (instr->kind) {
        case ir::InstrKind::ADD: 
            emit("add " + work_reg + ", " + src2_op); 
            break;
        case ir::InstrKind::SUB: 
            emit("sub " + work_reg + ", " + src2_op); 
            break;
        case ir::InstrKind::MUL: 
            emit("imul " + work_reg + ", " + src2_op); 
            break;
        case ir::InstrKind::NEG: 
            emit("neg " + work_reg); 
            break;
        default: 
            break;
    }
    
    if (dest_is_mem || work_reg != dest_op) {
        storeFromReg(work_reg, instr->dest);
    }
}

void X86Generator::generateComparison(const ir::Instruction* instr) {
    loadToReg("eax", instr->src1);
    emit("cmp eax, " + getOperand(instr->src2));
    
    switch (instr->kind) {
        case ir::InstrKind::CMP_EQ: emit("sete al");  break;
        case ir::InstrKind::CMP_NE: emit("setne al"); break;
        case ir::InstrKind::CMP_LT: emit("setl al");  break;
        case ir::InstrKind::CMP_LE: emit("setle al"); break;
        case ir::InstrKind::CMP_GT: emit("setg al");  break;
        case ir::InstrKind::CMP_GE: emit("setge al"); break;
        default: break;
    }
    
    emit("movzx eax, al");
    storeFromReg("eax", instr->dest);
}

void X86Generator::generateLogic(const ir::Instruction* instr) {
    std::string dest_op = getOperand(instr->dest);
    bool dest_is_mem = (dest_op.find("[rbp") != std::string::npos || 
                        dest_op[0] == '[');
    
    std::string work_reg = dest_is_mem ? "eax" : dest_op;
    loadToReg(work_reg, instr->src1);
    
    switch (instr->kind) {
        case ir::InstrKind::AND: 
            emit("and " + work_reg + ", " + getOperand(instr->src2)); 
            break;
        case ir::InstrKind::OR:  
            emit("or "  + work_reg + ", " + getOperand(instr->src2)); 
            break;
        case ir::InstrKind::NOT: 
            emit("not " + work_reg); 
            break;
        default: 
            break;
    }
    
    if (dest_is_mem) {
        storeFromReg(work_reg, instr->dest);
    }
}

void X86Generator::generateMemory(const ir::Instruction* instr) {
    switch (instr->kind) {
        case ir::InstrKind::LOAD:
            loadToReg("eax", instr->src1);
            storeFromReg("eax", instr->dest);
            break;
        case ir::InstrKind::STORE:
            loadToReg("eax", instr->src1);
            storeFromReg("eax", instr->dest);
            break;
        case ir::InstrKind::ALLOCA:
            break;
        default:
            break;
    }
}

void X86Generator::generateControlFlow(const ir::Instruction* instr) {
    switch (instr->kind) {
        case ir::InstrKind::JUMP:
            emit("jmp " + instr->target_label);
            current_block_ended = true;
            break;
            
        case ir::InstrKind::JUMP_IF: {
            std::string src_op = getOperand(instr->src1);
            if (src_op.find('[') == std::string::npos) {
                emit("test " + src_op + ", " + src_op);
            } else {
                loadToReg("eax", instr->src1);
                emit("test eax, eax");
            }
            emit("jnz " + instr->target_label);
            break;
        }
            
        case ir::InstrKind::JUMP_IF_NOT: {
            std::string src_op = getOperand(instr->src1);
            if (src_op.find('[') == std::string::npos) {
                emit("test " + src_op + ", " + src_op);
            } else {
                loadToReg("eax", instr->src1);
                emit("test eax, eax");
            }
            emit("jz " + instr->target_label);
            break;
        }
            
        case ir::InstrKind::LABEL:
            if (instr->dest.name != "entry") {
                emitLabel(instr->dest.name);
            }
            current_block_ended = false;
            break;
            
        default:
            break;
    }
}

void X86Generator::generateCall(const ir::Instruction* instr) {
    std::string func_name = instr->src1.name;
    
    size_t arg_count = instr->args.size();
    for (size_t i = 0; i < arg_count && i < 6; ++i) {
        std::string arg_reg = ABI::getArgRegister(static_cast<int>(i), false);
        std::string arg_reg_32 = ABI::getRegName(arg_reg, 4);
        loadToReg(arg_reg_32, instr->args[i]);
    }
    
    for (size_t i = 6; i < arg_count; ++i) {
        loadToReg("eax", instr->args[i]);
        emit("push rax");
    }
    
    emit("call " + func_name);
    
    if (arg_count > 6) {
        emit("add rsp, " + std::to_string((arg_count - 6) * 8));
    }
    
    last_call_dest = instr->dest.name;
    storeFromReg("eax", instr->dest);
}

void X86Generator::generateReturn(const ir::Instruction* instr) {
    if (instr->src1.kind != ir::OperandKind::TEMP || !instr->src1.name.empty()) {
        std::string src_op = getOperand(instr->src1);
        
        if (!last_call_dest.empty() && instr->src1.name == last_call_dest) {
        } else if (src_op == "eax") {
        } else if (src_op.find('[') != std::string::npos) {
            loadToReg("eax", instr->src1);
        } else {
            emit("mov eax, " + src_op);
        }
    }
    
    if (current_function) {
        generateEpilogue(current_function);
    }
    current_block_ended = true;
}

void X86Generator::generatePhi(const ir::Instruction* instr) {
    if (!instr->args.empty() && !instr->dest.name.empty()) {
        std::string phi_dest = instr->dest.name;
        std::string phi_base = extractBaseName(phi_dest);
        
        for (size_t i = 0; i < instr->args.size(); i += 2) {
            if (i + 1 < instr->args.size()) {
                const std::string& src_name = instr->args[i].name;
                std::string src_base = extractBaseName(src_name);
                
                auto it = var_mapping.find(src_name);
                if (it != var_mapping.end()) {
                    var_mapping[phi_dest] = it->second;
                    var_mapping[phi_base] = it->second;
                    return;
                }
                
                it = var_mapping.find(src_base);
                if (it != var_mapping.end()) {
                    var_mapping[phi_dest] = it->second;
                    var_mapping[phi_base] = it->second;
                    return;
                }
                
                if (current_frame && current_frame->hasVar(src_base)) {
                    int offset = current_frame->getVarOffset(src_base);
                    std::string mem = "dword [rbp" + std::to_string(offset) + "]";
                    var_mapping[phi_dest] = mem;
                    var_mapping[phi_base] = mem;
                    return;
                }
            }
        }
        
        if (current_frame && current_frame->hasVar(phi_base)) {
            int offset = current_frame->getVarOffset(phi_base);
            var_mapping[phi_dest] = "dword [rbp" + std::to_string(offset) + "]";
        }
    }
}

void X86Generator::generateMove(const ir::Instruction* instr) {
    std::string dest_op = getOperand(instr->dest);
    
    if (instr->src1.kind == ir::OperandKind::CONST_INT ||
        instr->src1.kind == ir::OperandKind::CONST_BOOL) {
        emit("mov " + dest_op + ", " + getOperand(instr->src1));
        return;
    }
    
    std::string src_op = getOperand(instr->src1);
    
    if (src_op.find('[') == std::string::npos && dest_op.find('[') == std::string::npos) {
        if (src_op != dest_op) {
            emit("mov " + dest_op + ", " + src_op);
        }
        return;
    }
    
    loadToReg("eax", instr->src1);
    storeFromReg("eax", instr->dest);
}

std::string X86Generator::generate(ir::IRProgram* program) {
    ir_program = program;
    out.str("");
    out.clear();
    data_section.str("");
    data_section.clear();
    bss_section.str("");
    bss_section.clear();
    rodata_section.str("");
    rodata_section.clear();
    
    out << "; x86-64 Assembly generated by compiler-project\n";
    out << "; System V AMD64 ABI compliant\n";
    
    if (use_register_allocation) {
        out << "; Register allocation: Linear Scan\n";
    }
    if (use_peephole_optimization) {
        out << "; Peephole optimization: enabled\n";
    }
    out << "\n";
    
    generateTextSection();
    
    std::string result = out.str();
    if (use_peephole_optimization) {
        PeepholeOptimizer peephole;
        result = peephole.optimize(result);
    }
    
    if (!data_section.str().empty()) {
        result += "\nsection .data\n";
        result += data_section.str();
    }
    
    if (!bss_section.str().empty()) {
        result += "\nsection .bss\n";
        result += bss_section.str();
    }
    
    if (!rodata_section.str().empty()) {
        result += "\nsection .rodata\n";
        result += rodata_section.str();
    }
    
    return result;
}

void X86Generator::generateTextSection() {
    out << "section .text\n\n";
    
    for (const auto& func : ir_program->getFunctions()) {
        generateFunction(func.get());
    }
}

void X86Generator::generateFunction(const ir::IRFunction* func) {
    current_function = func;
    var_mapping.clear();
    current_block_ended = false;
    last_call_dest = "";
    is_leaf_function = isLeafFunction(func);
    
    StackFrame frame = StackFrameAnalyzer::analyze(func);
    
    if (use_register_allocation) {
        reg_alloc.reset();
        reg_alloc.collectLiveRanges(func);
        reg_alloc.allocateRegisters(func);
        frame.setSpillSlotsSize(reg_alloc.getTotalSpillSize());
    }
    
    current_frame = &frame;
    
    out << "global " << func->getName() << "\n";
    emitLabel(func->getName());
    
    generatePrologue(func);
    
    for (const auto& block : func->getBlocks()) {
        current_block_ended = false;
        
        if (block->getLabel() != "entry") {
            emitLabel(block->getLabel());
        }
        
        for (const auto& instr : block->getInstructions()) {
            generateInstruction(instr.get());
        }
    }
    
    if (func->getReturnType() && func->getReturnType()->isVoid()) {
        if (!current_block_ended) {
            generateEpilogue(func);
        }
    }
    
    out << "\n";
}

void X86Generator::generateDataSection() {
}

void X86Generator::generateBssSection() {
}

void X86Generator::generateRodataSection() {
}

void X86Generator::generateGlobalVariable(const std::string& name, 
                                          const ir::Operand& value) {
    data_section << name << ": dd " << getOperand(value) << "\n";
}

void X86Generator::generateStringLiteral(const std::string& label, 
                                         const std::string& value) {
    rodata_section << label << ": db \"" << value << "\", 0\n";
}

}