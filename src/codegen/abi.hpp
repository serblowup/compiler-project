#ifndef CODEGEN_ABI_HPP
#define CODEGEN_ABI_HPP

#include <string>
#include <vector>
#include "../semantic/type.hpp"

namespace codegen {

class ABI {
public:
    static const std::vector<std::string> ARG_REGISTERS_INT;
    static const std::vector<std::string> ARG_REGISTERS_FLOAT;
    static const std::vector<std::string> CALLER_SAVED;
    static const std::vector<std::string> CALLEE_SAVED;
    
    static const std::string RETURN_REG_INT;
    static const std::string RETURN_REG_FLOAT;
    static const std::string STACK_PTR;
    static const std::string BASE_PTR;
    
    static constexpr int STACK_ALIGNMENT = 16;
    static constexpr int RED_ZONE_SIZE = 128;
    static constexpr int REGISTER_SIZE = 8;
    
    static int getTypeSize(semantic::Type* type);
    static int getTypeAlignment(semantic::Type* type);
    static std::string getRegName(const std::string& reg64, int size);
    static bool isArgInRegister(int index, bool is_float);
    static std::string getArgRegister(int index, bool is_float);
    static int getStackArgOffset(int index, bool is_float);
    
    static constexpr int SYS_WRITE = 1;
    static constexpr int SYS_READ = 0;
    static constexpr int SYS_EXIT = 60;
    static constexpr int STDOUT = 1;
    static constexpr int STDIN = 0;
};

}

#endif