#ifndef CODEGEN_ABI_HPP
#define CODEGEN_ABI_HPP

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include "../semantic/type.hpp"

namespace ir {
    class IRFunction;
    enum class OperandKind;
    enum class InstrKind;
}

namespace codegen {

// Константы System V AMD64 ABI
class ABI {
public:
    // Регистры для передачи целочисленных аргументов
    static const std::vector<std::string> ARG_REGISTERS_INT;
    
    // Регистры для передачи float/double аргументов
    static const std::vector<std::string> ARG_REGISTERS_FLOAT;
    
    // Caller-saved регистры
    static const std::vector<std::string> CALLER_SAVED;
    
    // Callee-saved регистры
    static const std::vector<std::string> CALLEE_SAVED;
    
    // Специальные регистры
    static const std::string RETURN_REG_INT; // rax
    static const std::string RETURN_REG_FLOAT; // xmm0
    static const std::string STACK_PTR; // rsp
    static const std::string BASE_PTR; // rbp
    
    // Размеры
    static constexpr int STACK_ALIGNMENT = 16;
    static constexpr int RED_ZONE_SIZE = 128;
    static constexpr int REGISTER_SIZE = 8;
    
    // Размеры типов в байтах
    static int getTypeSize(semantic::Type* type);
    static int getTypeAlignment(semantic::Type* type);
    
    // Получение имени 32/64/8-битного регистра
    static std::string getRegName(const std::string& reg64, int size);
    
    // Проверка, помещается ли аргумент в регистры
    static bool isArgInRegister(int index, bool is_float);
    
    // Получение имени регистра для аргумента по индексу
    static std::string getArgRegister(int index, bool is_float);
    
    // Получение позиции аргумента в стеке
    static int getStackArgOffset(int index, bool is_float);
    
    // Системные вызовы Linux
    static constexpr int SYS_WRITE = 1;
    static constexpr int SYS_READ = 0;
    static constexpr int SYS_EXIT = 60;
    static constexpr int STDOUT = 1;
    static constexpr int STDIN = 0;
};

// Класс для распределения регистров методом Linear Scan
class RegisterAllocator {
public:
    struct LiveRange {
        std::string var_name;
        int start;
        int end;
        bool is_spilled;
        int spill_slot;
        
        LiveRange() : start(0), end(0), is_spilled(false), spill_slot(0) {}
        
        bool operator<(const LiveRange& other) const {
            if (start != other.start) return start < other.start;
            return end < other.end;
        }
    };
    
    // Активный интервал
    struct ActiveInterval {
        std::string var_name;
        int end;
        std::string reg;
        
        // Для сравнения
        bool operator<(const ActiveInterval& other) const {
            return end < other.end;
        }
    };

private:
    // Доступные 32-битные регистры
    std::vector<std::string> gp_regs_32;
    
    // Их 64-битные версии
    std::vector<std::string> gp_regs_64;
    
    // Состояние регистров
    struct RegState {
        bool in_use;
        std::string var_name;
        
        RegState() : in_use(false), var_name("") {}
    };
    std::unordered_map<std::string, RegState> reg_states;
    
    std::vector<LiveRange> live_ranges;
    
    std::unordered_map<std::string, std::string> var_to_reg;
    
    std::unordered_map<std::string, std::string> var_to_mem;
    
    int next_spill_offset;
    int max_spill_offset;
    
    bool allocation_done;
    
    // Статистика
    int total_vars;
    int vars_in_regs;
    int vars_spilled;

public:
    RegisterAllocator();
    
    // Сбор диапазонов
    void collectLiveRanges(const ir::IRFunction* func);
    
    // Распределение регистров
    void allocateRegisters(const ir::IRFunction* func);
    
    // Запрос результатов
    std::string getReg(const std::string& var_name) const;
    std::string getMem(const std::string& var_name) const;
    bool isInRegister(const std::string& var_name) const;
    bool isSpilled(const std::string& var_name) const;
    
    // Информация о spill
    int getSpillSlotOffset(const std::string& var_name) const;
    int getTotalSpillSize() const;

    // Статистика
    void printStats() const;
    
    // Сброс состояния
    void reset();
    
private:
    // Освободить регистры, чьи интервалы истекли
    void expireOldIntervals(int current_pos, 
                           std::vector<ActiveInterval>& active);
    
    // Найти или создать диапазон для переменной
    void addVarUse(const std::string& var_name, int instr_index);
    
    // Извлечь базовое имя переменной (для SSA)
    static std::string extractBaseName(const std::string& ssa_name);
};

}

#endif