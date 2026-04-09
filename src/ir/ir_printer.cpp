#include "ir_printer.hpp"
#include <iomanip>
#include <algorithm>

namespace ir {

// Текстовый вывод
std::string IRPrinter::toString(IRProgram* program) {
    if (!program) return "";
    return program->toString();
}

// DOT
std::string IRPrinter::escapeDOT(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

std::string IRPrinter::toDot(IRProgram* program) {
    if (!program) return "";
    
    std::ostringstream oss;
    oss << "digraph CFG {\n";
    oss << "  node [shape=box, fontname=\"Courier\", fontsize=10];\n";
    oss << "  edge [fontname=\"Courier\", fontsize=8];\n\n";
    
    int cluster_num = 0;
    
    for (const auto& func : program->getFunctions()) {
        oss << "  subgraph cluster_" << cluster_num++ << " {\n";
        oss << "    label=\"" << escapeDOT(func->getName()) << "\";\n";
        oss << "    style=filled;\n";
        oss << "    fillcolor=lightgrey;\n\n";
        
        // Узлы для каждого базового блока
        for (const auto& block : func->getBlocks()) {
            std::string block_label = block->getLabel();
            
            // Содержимое блока
            std::string content;
            for (const auto& instr : block->getInstructions()) {
                if (!content.empty()) content += "\\l";
                content += instr->toString();
            }
            
            oss << "    " << block_label << " [label=\"" 
                << escapeDOT(block_label) << ":\\l" 
                << escapeDOT(content) << "\", shape=box];\n";
        }
        
        oss << "\n";
        
        // Рёбра между блоками
        for (const auto& block : func->getBlocks()) {
            for (BasicBlock* succ : block->getSuccessors()) {
                oss << "    " << block->getLabel() << " -> " 
                    << succ->getLabel() << ";\n";
            }
        }
        
        oss << "  }\n\n";
    }
    
    oss << "}\n";
    return oss.str();
}

// JSON
std::string IRPrinter::escapeJSON(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default: result += c;
        }
    }
    return result;
}

std::string IRPrinter::toJSON(IRProgram* program) {
    if (!program) return "{}";
    
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"program\": {\n";
    oss << "    \"functions\": [\n";
    
    const auto& functions = program->getFunctions();
    for (size_t f = 0; f < functions.size(); ++f) {
        const auto& func = functions[f];
        
        oss << "      {\n";
        oss << "        \"name\": \"" << escapeJSON(func->getName()) << "\",\n";
        oss << "        \"return_type\": \"" 
            << escapeJSON(func->getReturnType() ? func->getReturnType()->toString() : "void") << "\",\n";
        oss << "        \"parameters\": [\n";
        
        const auto& params = func->getParameters();
        for (size_t p = 0; p < params.size(); ++p) {
            oss << "          {\"name\": \"" << escapeJSON(params[p].name) 
                << "\", \"type\": \"" << escapeJSON(params[p].type->toString()) << "\"}";
            if (p < params.size() - 1) oss << ",";
            oss << "\n";
        }
        
        oss << "        ],\n";
        oss << "        \"basic_blocks\": [\n";
        
        const auto& blocks = func->getBlocks();
        for (size_t b = 0; b < blocks.size(); ++b) {
            const auto& block = blocks[b];
            
            oss << "          {\n";
            oss << "            \"label\": \"" << escapeJSON(block->getLabel()) << "\",\n";
            oss << "            \"instructions\": [\n";
            
            const auto& instrs = block->getInstructions();
            for (size_t i = 0; i < instrs.size(); ++i) {
                oss << "              \"" << escapeJSON(instrs[i]->toString()) << "\"";
                if (i < instrs.size() - 1) oss << ",";
                oss << "\n";
            }
            
            oss << "            ],\n";
            oss << "            \"successors\": [\n";
            
            const auto& succs = block->getSuccessors();
            for (size_t s = 0; s < succs.size(); ++s) {
                oss << "              \"" << escapeJSON(succs[s]->getLabel()) << "\"";
                if (s < succs.size() - 1) oss << ",";
                oss << "\n";
            }
            
            oss << "            ]\n";
            oss << "          }";
            if (b < blocks.size() - 1) oss << ",";
            oss << "\n";
        }
        
        oss << "        ]\n";
        oss << "      }";
        if (f < functions.size() - 1) oss << ",";
        oss << "\n";
    }
    
    oss << "    ]\n";
    oss << "  }\n";
    oss << "}\n";
    
    return oss.str();
}

// Статистика
void IRPrinter::Stats::addInstr(InstrKind kind) {
    total_instructions++;
    instr_counts[kind]++;
}

std::string IRPrinter::Stats::toString() const {
    std::ostringstream oss;
    
    oss << "IR Statistics:\n";
    oss << "  Total instructions: " << total_instructions << "\n";
    oss << "  Total basic blocks: " << total_blocks << "\n";
    oss << "  Total temporaries:  " << total_temps << "\n";
    oss << "  Max stack depth:    " << max_stack_depth << "\n\n";
    
    oss << "  Instruction breakdown:\n";
    
    // Список инструкций в порядке значимости
    std::vector<std::pair<InstrKind, int>> sorted;
    for (const auto& pair : instr_counts) {
        sorted.push_back(pair);
    }
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& pair : sorted) {
        oss << "    " << std::setw(12) << instrKindToString(pair.first) 
            << ": " << pair.second << "\n";
    }
    
    return oss.str();
}

IRPrinter::Stats IRPrinter::collectStats(IRProgram* program) {
    Stats stats;
    
    if (!program) return stats;
    
    std::unordered_map<std::string, int> temp_uses;
    
    for (const auto& func : program->getFunctions()) {
        for (const auto& block : func->getBlocks()) {
            stats.addBlock();
            
            for (const auto& instr : block->getInstructions()) {
                stats.addInstr(instr->kind);
                
                // Собираем временные переменные
                if (instr->dest.kind == OperandKind::TEMP) {
                    temp_uses[instr->dest.name]++;
                }
                if (instr->src1.kind == OperandKind::TEMP) {
                    temp_uses[instr->src1.name]++;
                }
                if (instr->src2.kind == OperandKind::TEMP) {
                    temp_uses[instr->src2.name]++;
                }
            }
        }
        
        stats.total_temps += func->getLocalVars().size();
        stats.max_stack_depth = std::max(stats.max_stack_depth, func->getStackSize());
    }
    
    return stats;
}

std::string IRPrinter::getStats(IRProgram* program) {
    Stats stats = collectStats(program);
    return stats.toString();
}

// Дополнительно: вывод с комментариями исходного кода
std::string IRPrinter::toStringWithSource(IRProgram* program, const std::string& source) {
    std::ostringstream oss;
    oss << "# IR Program\n";
    oss << "# Source:\n";
    
    // Добавляем исходный код
    std::istringstream source_iss(source);
    std::string line;
    int line_num = 1;
    while (std::getline(source_iss, line) && line_num <= 20) {
        oss << "# " << std::setw(3) << line_num << " " << line << "\n";
        line_num++;
    }
    if (line_num <= 20) {
        oss << "# ...\n";
    }
    
    oss << "\n" << program->toString();
    
    return oss.str();
}

}