#ifndef IR_PRINTER_HPP
#define IR_PRINTER_HPP

#include "ir.hpp"
#include <string>
#include <sstream>

namespace ir {

class IRPrinter {
public:
    // Текстовый вывод
    static std::string toString(IRProgram* program);
    
    // DOT
    static std::string toDot(IRProgram* program);
    
    // JSON
    static std::string toJSON(IRProgram* program);
    
    // Статистика
    static std::string getStats(IRProgram* program);
    
    // Вывод с комментариями src
    static std::string toStringWithSource(IRProgram* program, const std::string& source);
 
private:
    static std::string escapeJSON(const std::string& str);
    static std::string escapeDOT(const std::string& str);
    
    struct Stats {
        int total_instructions = 0;
        int total_blocks = 0;
        int total_temps = 0;
        int max_stack_depth = 0;
        
        std::unordered_map<InstrKind, int> instr_counts;
        
        void addInstr(InstrKind kind);
        void addBlock() { total_blocks++; }
        std::string toString() const;
    };
    
    static Stats collectStats(IRProgram* program);
};

}

#endif
