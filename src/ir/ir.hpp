#ifndef IR_HPP
#define IR_HPP

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <unordered_map>
#include <optional>
#include "../semantic/type.hpp"

namespace ir {

/* Типы операндов:
 * - виртуальный регистр
 * - имя переменной
 * - константа int
 * - константа float
 * - константа bool
 * - метка базового блока
 * - память
 */
enum class OperandKind {
    TEMP,
    VARIABLE,
    CONST_INT,
    CONST_FLOAT,
    CONST_BOOL,
    LABEL,
    MEMORY
};

struct Operand {
    OperandKind kind;
    
    std::string name;
    
    int int_value;
    
    float float_value;
    
    bool bool_value;
    
    std::string mem_base;
    int mem_offset;
    
    Operand() : kind(OperandKind::TEMP), int_value(0), float_value(0.0f), bool_value(false), mem_offset(0) {}
    
    static Operand Temp(const std::string& name) {
        Operand op;
        op.kind = OperandKind::TEMP;
        op.name = name;
        return op;
    }
    
    static Operand Var(const std::string& name) {
        Operand op;
        op.kind = OperandKind::VARIABLE;
        op.name = name;
        return op;
    }
    
    static Operand ConstInt(int value) {
        Operand op;
        op.kind = OperandKind::CONST_INT;
        op.int_value = value;
        return op;
    }
    
    static Operand ConstFloat(float value) {
        Operand op;
        op.kind = OperandKind::CONST_FLOAT;
        op.float_value = value;
        return op;
    }
    
    static Operand ConstBool(bool value) {
        Operand op;
        op.kind = OperandKind::CONST_BOOL;
        op.bool_value = value;
        return op;
    }
    
    static Operand Label(const std::string& name) {
        Operand op;
        op.kind = OperandKind::LABEL;
        op.name = name;
        return op;
    }
    
    static Operand Memory(const std::string& base, int offset = 0) {
        Operand op;
        op.kind = OperandKind::MEMORY;
        op.mem_base = base;
        op.mem_offset = offset;
        return op;
    }
    
    std::string toString() const;
    bool isConstant() const;
};

/* Инструкции:
 * - ариметические
 * - логические
 * - сравнение
 * - работа с памятью
 * - поток управления
 * - функции
 */
enum class InstrKind {
    ADD, SUB, MUL, DIV, MOD, NEG,
    AND, OR, NOT,
    CMP_EQ, CMP_NE, CMP_LT, CMP_LE, CMP_GT, CMP_GE,
    LOAD, STORE, ALLOCA,
    JUMP, JUMP_IF, JUMP_IF_NOT, LABEL, PHI,
    CALL, RETURN, PARAM,
    MOVE
};

struct Instruction {
    InstrKind kind;
    Operand dest;
    Operand src1;
    Operand src2;
    std::vector<Operand> args;
    std::string target_label;
    semantic::Type* type;
    
    Instruction(InstrKind k);
    
    std::string toString() const;
    bool isTerminator() const;
};

// Базовый блок
class BasicBlock {
    std::string label;
    std::vector<std::unique_ptr<Instruction>> instructions;
    std::vector<BasicBlock*> predecessors;
    std::vector<BasicBlock*> successors;
    
public:
    explicit BasicBlock(const std::string& lbl);
    
    void addInstruction(std::unique_ptr<Instruction> instr);
    void addSuccessor(BasicBlock* block);
    void addPredecessor(BasicBlock* block);
    
    const std::string& getLabel() const;
    const std::vector<std::unique_ptr<Instruction>>& getInstructions() const;
    const std::vector<BasicBlock*>& getSuccessors() const;
    const std::vector<BasicBlock*>& getPredecessors() const;
    
    std::vector<std::unique_ptr<Instruction>>& getInstructionsMutable();
    void removeInstruction(size_t index);
    
    std::string toString() const;
};

// Функция
struct ParameterInfo {
    std::string name;
    semantic::Type* type;
};

struct LocalVarInfo {
    std::string name;
    semantic::Type* type;
    int stack_offset;
};

class IRFunction {
    std::string name;
    semantic::Type* return_type;
    std::vector<ParameterInfo> parameters;
    std::vector<LocalVarInfo> local_vars;
    std::vector<std::unique_ptr<BasicBlock>> blocks;
    BasicBlock* entry_block;
    int stack_size;
    
public:
    explicit IRFunction(const std::string& n, semantic::Type* ret_type = nullptr);
    
    void setReturnType(semantic::Type* type);
    void addParameter(const std::string& name, semantic::Type* type);
    void addLocalVar(const std::string& name, semantic::Type* type, int offset = -1);
    void addBlock(std::unique_ptr<BasicBlock> block);
    void setEntryBlock(BasicBlock* block);
    BasicBlock* createBlock(const std::string& label);
    void setStackSize(int size);
    
    const std::string& getName() const;
    semantic::Type* getReturnType() const;
    const std::vector<ParameterInfo>& getParameters() const;
    const std::vector<LocalVarInfo>& getLocalVars() const;
    const std::vector<std::unique_ptr<BasicBlock>>& getBlocks() const;
    BasicBlock* getEntryBlock() const;
    int getStackSize() const;
    
    std::vector<std::unique_ptr<BasicBlock>>& getBlocksMutable();
    
    std::string toString() const;
};

// Программа
class IRProgram {
    std::vector<std::unique_ptr<IRFunction>> functions;
    std::unordered_map<std::string, IRFunction*> function_map;
    
public:
    void addFunction(std::unique_ptr<IRFunction> func);
    IRFunction* getFunction(const std::string& name);
    const std::vector<std::unique_ptr<IRFunction>>& getFunctions() const;
    bool hasFunction(const std::string& name) const;
    
    void clear();
    
    std::string toString() const;
};

const char* instrKindToString(InstrKind kind);
const char* operandKindToString(OperandKind kind);

}

#endif
