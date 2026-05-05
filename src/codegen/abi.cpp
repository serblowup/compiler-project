#include "abi.hpp"
#include <algorithm>

namespace codegen {

const std::vector<std::string> ABI::ARG_REGISTERS_INT = {
    "rdi", "rsi", "rdx", "rcx", "r8", "r9"
};

const std::vector<std::string> ABI::ARG_REGISTERS_FLOAT = {
    "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7"
};

const std::vector<std::string> ABI::CALLER_SAVED = {
    "rax", "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", "r11"
};

const std::vector<std::string> ABI::CALLEE_SAVED = {
    "rbx", "rsp", "rbp", "r12", "r13", "r14", "r15"
};

const std::string ABI::RETURN_REG_INT = "rax";
const std::string ABI::RETURN_REG_FLOAT = "xmm0";
const std::string ABI::STACK_PTR = "rsp";
const std::string ABI::BASE_PTR = "rbp";

int ABI::getTypeSize(semantic::Type* type) {
    if (!type) return 4;
    
    switch (type->getKind()) {
        case semantic::TypeKind::INT:    return 4;
        case semantic::TypeKind::FLOAT:  return 4;
        case semantic::TypeKind::BOOL:   return 1;
        case semantic::TypeKind::VOID:   return 0;
        case semantic::TypeKind::STRING: return 8;
        case semantic::TypeKind::STRUCT: {
            auto info = type->getStructInfo();
            if (!info) return 8;
            int total = 0;
            for (const auto& field : info->fields) {
                total += getTypeSize(field.second);
            }
            return total;
        }
        default: return 8;
    }
}

int ABI::getTypeAlignment(semantic::Type* type) {
    if (!type) return 4;
    
    switch (type->getKind()) {
        case semantic::TypeKind::INT:    return 4;
        case semantic::TypeKind::FLOAT:  return 4;
        case semantic::TypeKind::BOOL:   return 1;
        case semantic::TypeKind::STRING: return 8;
        default: return 8;
    }
}

std::string ABI::getRegName(const std::string& reg64, int size) {
    if (size == 1) {
        if (reg64 == "rax") return "al";
        if (reg64 == "rbx") return "bl";
        if (reg64 == "rcx") return "cl";
        if (reg64 == "rdx") return "dl";
        if (reg64 == "rsi") return "sil";
        if (reg64 == "rdi") return "dil";
        if (reg64 == "rbp") return "bpl";
        if (reg64 == "rsp") return "spl";
        if (reg64 == "r8")  return "r8b";
        if (reg64 == "r9")  return "r9b";
        if (reg64 == "r10") return "r10b";
        if (reg64 == "r11") return "r11b";
        if (reg64 == "r12") return "r12b";
        if (reg64 == "r13") return "r13b";
        if (reg64 == "r14") return "r14b";
        if (reg64 == "r15") return "r15b";
        return reg64;
    } else if (size == 4) {
        std::string r = reg64;
        if (r.length() >= 2 && r[0] == 'r') {
            r[0] = 'e';
            return r;
        }
        if (r == "r8")  return "r8d";
        if (r == "r9")  return "r9d";
        if (r == "r10") return "r10d";
        if (r == "r11") return "r11d";
        if (r == "r12") return "r12d";
        if (r == "r13") return "r13d";
        if (r == "r14") return "r14d";
        if (r == "r15") return "r15d";
        return reg64;
    }
    return reg64;
}

bool ABI::isArgInRegister(int index, bool is_float) {
    if (is_float) {
        return index < static_cast<int>(ARG_REGISTERS_FLOAT.size());
    }
    return index < static_cast<int>(ARG_REGISTERS_INT.size());
}

std::string ABI::getArgRegister(int index, bool is_float) {
    if (is_float) {
        if (index >= 0 && index < static_cast<int>(ARG_REGISTERS_FLOAT.size())) {
            return ARG_REGISTERS_FLOAT[index];
        }
    } else {
        if (index >= 0 && index < static_cast<int>(ARG_REGISTERS_INT.size())) {
            return ARG_REGISTERS_INT[index];
        }
    }
    return "";
}

int ABI::getStackArgOffset(int index, bool is_float) {
    int reg_count = is_float ? static_cast<int>(ARG_REGISTERS_FLOAT.size()) 
                             : static_cast<int>(ARG_REGISTERS_INT.size());
    
    int stack_index = index - reg_count;
    if (stack_index >= 0) {
        return 16 + (stack_index * 8);
    }
    return -1;
}

}