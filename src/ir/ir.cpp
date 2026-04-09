#include "ir.hpp"
#include <sstream>
#include <iomanip>

namespace ir {

// Операнд
std::string Operand::toString() const {
    std::ostringstream oss;
    
    switch (kind) {
        case OperandKind::TEMP:
            oss << name;
            break;
        case OperandKind::VARIABLE:
            oss << "[" << name << "]";
            break;
        case OperandKind::CONST_INT:
            oss << int_value;
            break;
        case OperandKind::CONST_FLOAT:
            oss << std::fixed << std::setprecision(6) << float_value;
            break;
        case OperandKind::CONST_BOOL:
            oss << (bool_value ? "true" : "false");
            break;
        case OperandKind::LABEL:
            oss << name;
            break;
        case OperandKind::MEMORY:
            if (mem_offset == 0) {
                oss << "[" << mem_base << "]";
            } else if (mem_offset > 0) {
                oss << "[" << mem_base << "+" << mem_offset << "]";
            } else {
                oss << "[" << mem_base << mem_offset << "]";
            }
            break;
    }
    
    return oss.str();
}

bool Operand::isConstant() const {
    return kind == OperandKind::CONST_INT ||
           kind == OperandKind::CONST_FLOAT ||
           kind == OperandKind::CONST_BOOL;
}

// Инструкция
Instruction::Instruction(InstrKind k)
    : kind(k)
    , type(nullptr) {}

const char* instrKindToString(InstrKind kind) {
    switch (kind) {
        case InstrKind::ADD: return "ADD";
        case InstrKind::SUB: return "SUB";
        case InstrKind::MUL: return "MUL";
        case InstrKind::DIV: return "DIV";
        case InstrKind::MOD: return "MOD";
        case InstrKind::NEG: return "NEG";
        case InstrKind::AND: return "AND";
        case InstrKind::OR:  return "OR";
        case InstrKind::NOT: return "NOT";
        case InstrKind::CMP_EQ: return "CMP_EQ";
        case InstrKind::CMP_NE: return "CMP_NE";
        case InstrKind::CMP_LT: return "CMP_LT";
        case InstrKind::CMP_LE: return "CMP_LE";
        case InstrKind::CMP_GT: return "CMP_GT";
        case InstrKind::CMP_GE: return "CMP_GE";
        case InstrKind::LOAD: return "LOAD";
        case InstrKind::STORE: return "STORE";
        case InstrKind::ALLOCA: return "ALLOCA";
        case InstrKind::JUMP: return "JUMP";
        case InstrKind::JUMP_IF: return "JUMP_IF";
        case InstrKind::JUMP_IF_NOT: return "JUMP_IF_NOT";
        case InstrKind::LABEL: return "LABEL";
        case InstrKind::PHI: return "PHI";
        case InstrKind::CALL: return "CALL";
        case InstrKind::RETURN: return "RETURN";
        case InstrKind::PARAM: return "PARAM";
        case InstrKind::MOVE: return "MOVE";
        default: return "UNKNOWN";
    }
}

const char* operandKindToString(OperandKind kind) {
    switch (kind) {
        case OperandKind::TEMP: return "temp";
        case OperandKind::VARIABLE: return "var";
        case OperandKind::CONST_INT: return "const_int";
        case OperandKind::CONST_FLOAT: return "const_float";
        case OperandKind::CONST_BOOL: return "const_bool";
        case OperandKind::LABEL: return "label";
        case OperandKind::MEMORY: return "mem";
        default: return "unknown";
    }
}

std::string Instruction::toString() const {
    std::ostringstream oss;
    
    // Метка
    if (kind == InstrKind::LABEL) {
        oss << dest.toString() << ":";
        return oss.str();
    }
    
    // Безусловный переход
    if (kind == InstrKind::JUMP) {
        oss << "JUMP " << target_label;
        return oss.str();
    }
    
    // Условный переход (если true)
    if (kind == InstrKind::JUMP_IF) {
        oss << "JUMP_IF " << src1.toString() << ", " << target_label;
        return oss.str();
    }
    
    // Условный переход (если false)
    if (kind == InstrKind::JUMP_IF_NOT) {
        oss << "JUMP_IF_NOT " << src1.toString() << ", " << target_label;
        return oss.str();
    }
    
    // Return
    if (kind == InstrKind::RETURN) {
        if (src1.kind == OperandKind::TEMP || src1.kind == OperandKind::VARIABLE) {
            oss << "RETURN " << src1.toString();
        } else {
            oss << "RETURN";
        }
        return oss.str();
    }
    
    // Store
    if (kind == InstrKind::STORE) {
        oss << "STORE " << src1.toString() << ", " << dest.toString();
        return oss.str();
    }
    
    // Load
    if (kind == InstrKind::LOAD) {
        oss << "LOAD " << dest.toString() << ", " << src1.toString();
        return oss.str();
    }
    
    // Alloca
    if (kind == InstrKind::ALLOCA) {
        oss << dest.toString() << " = ALLOCA " << src1.toString();
        return oss.str();
    }
    
    // Call
    if (kind == InstrKind::CALL) {
        oss << dest.toString() << " = CALL " << src1.toString();
        for (const auto& arg : args) {
            oss << ", " << arg.toString();
        }
        return oss.str();
    }
    
    // PHI
    if (kind == InstrKind::PHI) {
        oss << dest.toString() << " = PHI";
        for (size_t i = 0; i < args.size(); i += 2) {
            if (i > 0) oss << ",";
            oss << " (" << args[i].toString() << ", " << args[i+1].toString() << ")";
        }
        return oss.str();
    }
    
    // PARAM
    if (kind == InstrKind::PARAM) {
        oss << "PARAM " << src1.toString() << ", " << src2.toString();
        return oss.str();
    }
    
    // Унарные операции
    if (kind == InstrKind::NEG || kind == InstrKind::NOT) {
        oss << dest.toString() << " = " << instrKindToString(kind)
            << " " << src1.toString();
        return oss.str();
    }
    
    // Бинарные операции
    if (kind == InstrKind::MOVE) {
        oss << dest.toString() << " = " << src1.toString();
    } else {
        oss << dest.toString() << " = " << instrKindToString(kind)
            << " " << src1.toString() << ", " << src2.toString();
    }
    
    // Комментарий с типом
    if (type && !type->isError()) {
        oss << "  # " << type->toString();
    }
    
    return oss.str();
}

bool Instruction::isTerminator() const {
    return kind == InstrKind::JUMP ||
           kind == InstrKind::JUMP_IF ||
           kind == InstrKind::JUMP_IF_NOT ||
           kind == InstrKind::RETURN;
}

// Базовый блок
BasicBlock::BasicBlock(const std::string& lbl)
    : label(lbl) {}

void BasicBlock::addInstruction(std::unique_ptr<Instruction> instr) {
    instructions.push_back(std::move(instr));
}

void BasicBlock::addSuccessor(BasicBlock* block) {
    successors.push_back(block);
}

void BasicBlock::addPredecessor(BasicBlock* block) {
    predecessors.push_back(block);
}

const std::string& BasicBlock::getLabel() const {
    return label;
}

const std::vector<std::unique_ptr<Instruction>>& BasicBlock::getInstructions() const {
    return instructions;
}

const std::vector<BasicBlock*>& BasicBlock::getSuccessors() const {
    return successors;
}

const std::vector<BasicBlock*>& BasicBlock::getPredecessors() const {
    return predecessors;
}

std::vector<std::unique_ptr<Instruction>>& BasicBlock::getInstructionsMutable() {
    return instructions;
}

void BasicBlock::removeInstruction(size_t index) {
    if (index < instructions.size()) {
        instructions.erase(instructions.begin() + index);
    }
}

std::string BasicBlock::toString() const {
    std::ostringstream oss;
    oss << label << ":\n";
    for (const auto& instr : instructions) {
        oss << "    " << instr->toString() << "\n";
    }
    return oss.str();
}

// IRFunction
IRFunction::IRFunction(const std::string& n, semantic::Type* ret_type)
    : name(n)
    , return_type(ret_type)
    , entry_block(nullptr)
    , stack_size(0) {}

void IRFunction::setReturnType(semantic::Type* type) {
    return_type = type;
}

void IRFunction::addParameter(const std::string& name, semantic::Type* type) {
    parameters.push_back({name, type});
}

void IRFunction::addLocalVar(const std::string& name, semantic::Type* type, int offset) {
    local_vars.push_back({name, type, offset});
}

void IRFunction::addBlock(std::unique_ptr<BasicBlock> block) {
    blocks.push_back(std::move(block));
}

void IRFunction::setEntryBlock(BasicBlock* block) {
    entry_block = block;
}

BasicBlock* IRFunction::createBlock(const std::string& label) {
    auto block = std::make_unique<BasicBlock>(label);
    BasicBlock* ptr = block.get();
    blocks.push_back(std::move(block));
    return ptr;
}

void IRFunction::setStackSize(int size) {
    stack_size = size;
}

const std::string& IRFunction::getName() const {
    return name;
}

semantic::Type* IRFunction::getReturnType() const {
    return return_type;
}

const std::vector<ParameterInfo>& IRFunction::getParameters() const {
    return parameters;
}

const std::vector<LocalVarInfo>& IRFunction::getLocalVars() const {
    return local_vars;
}

const std::vector<std::unique_ptr<BasicBlock>>& IRFunction::getBlocks() const {
    return blocks;
}

// Реализация метода для получения не-const доступа к блокам
std::vector<std::unique_ptr<BasicBlock>>& IRFunction::getBlocksMutable() {
    return blocks;
}

BasicBlock* IRFunction::getEntryBlock() const {
    return entry_block;
}

int IRFunction::getStackSize() const {
    return stack_size;
}

std::string IRFunction::toString() const {
    std::ostringstream oss;
    
    oss << "function " << name << ": ";
    oss << (return_type ? return_type->toString() : "void");
    oss << " (";
    
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << parameters[i].name << ": " << parameters[i].type->toString();
    }
    oss << ")\n";
    
    for (const auto& block : blocks) {
        std::string block_str = block->toString();
        std::istringstream iss(block_str);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "  " << line << "\n";
        }
    }
    
    return oss.str();
}

// IRProgram
void IRProgram::addFunction(std::unique_ptr<IRFunction> func) {
    function_map[func->getName()] = func.get();
    functions.push_back(std::move(func));
}

IRFunction* IRProgram::getFunction(const std::string& name) {
    auto it = function_map.find(name);
    return it != function_map.end() ? it->second : nullptr;
}

const std::vector<std::unique_ptr<IRFunction>>& IRProgram::getFunctions() const {
    return functions;
}

bool IRProgram::hasFunction(const std::string& name) const {
    return function_map.find(name) != function_map.end();
}

void IRProgram::clear() {
    functions.clear();
    function_map.clear();
}

std::string IRProgram::toString() const {
    std::ostringstream oss;
    oss << "# IR Program\n";
    oss << "# Generated by compiler-project\n\n";
    
    for (const auto& func : functions) {
        oss << func->toString() << "\n";
    }
    
    return oss.str();
}

}
