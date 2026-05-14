#include "peephole_optimizer.hpp"
#include <algorithm>
#include <cctype>
#include <iostream>

namespace codegen {

PeepholeOptimizer::PeepholeOptimizer() {
    resetStats();
}

void PeepholeOptimizer::resetStats() {
    stats = OptimizationStats();
}

const PeepholeOptimizer::OptimizationStats& PeepholeOptimizer::getStats() const {
    return stats;
}

std::string PeepholeOptimizer::OptimizationStats::toString() const {
    std::ostringstream oss;
    oss << "[Peephole Optimization Statistics]\n";
    oss << "  Redundant moves removed:  " << redundant_moves_removed << "\n";
    oss << "  Redundant loads removed:  " << redundant_loads_removed << "\n";
    oss << "  inc/dec replacements:     " << inc_dec_replacements << "\n";
    oss << "  xor zero replacements:    " << xor_zero_replacements << "\n";
    oss << "  test before jump removed: " << test_removed << "\n";
    oss << "  self moves removed:       " << self_moves_removed << "\n";
    oss << "  mul→shl replacements:     " << mul_to_shift_replacements << "\n";
    oss << "  useless jumps removed:    " << useless_jumps_removed << "\n";
    oss << "  Total instructions removed: " << total_removed << "\n";
    return oss.str();
}

PeepholeOptimizer::AsmLine PeepholeOptimizer::parseLine(const std::string& line) {
    AsmLine result;
    result.raw = line;
    
    std::string trimmed = line;
    
    size_t start = 0;
    while (start < trimmed.size() && (trimmed[start] == ' ' || trimmed[start] == '\t')) {
        start++;
    }
    
    if (start >= trimmed.size()) {
        return result;
    }
    
    trimmed = trimmed.substr(start);
    bool has_indent = (start > 0);
    
    if (trimmed[0] == ';') {
        result.comment = trimmed.substr(1);
        size_t ns = 0;
        while (ns < result.comment.size() && (result.comment[ns] == ' ' || result.comment[ns] == '\t')) {
            ns++;
        }
        if (ns > 0) result.comment = result.comment.substr(ns);
        result.has_indent = has_indent;
        return result;
    }
    
    if (trimmed[0] == '.' || trimmed.substr(0,6) == "global" || trimmed.substr(0,7) == "section") {
        result.is_directive = true;
        result.raw = trimmed;
        return result;
    }
    
    bool has_colon = (trimmed.find(':') != std::string::npos);
    bool starts_with_alpha = std::isalpha(trimmed[0]) || trimmed[0] == '_' || trimmed[0] == '.';
    
    if (has_colon && starts_with_alpha && !has_indent) {
        size_t colon_pos = trimmed.find(':');

        bool is_label = true;
        for (size_t i = 0; i < colon_pos; ++i) {
            char c = trimmed[i];
            if (c == ' ' || c == '\t' || c == '[' || c == ']' || c == ',') {
                is_label = false;
                break;
            }
        }
        
        if (is_label) {
            result.is_label = true;
            result.label = trimmed.substr(0, colon_pos + 1);
            
            std::string after_label = trimmed.substr(colon_pos + 1);
            size_t ns = 0;
            while (ns < after_label.size() && (after_label[ns] == ' ' || after_label[ns] == '\t')) {
                ns++;
            }
            after_label = after_label.substr(ns);
            
            if (!after_label.empty() && after_label[0] != ';') {
                result.raw = after_label;
            } else if (!after_label.empty()) {
                result.comment = after_label.substr(1);
                ns = 0;
                while (ns < result.comment.size() && (result.comment[ns] == ' ' || result.comment[ns] == '\t')) {
                    ns++;
                }
                if (ns > 0) result.comment = result.comment.substr(ns);
            }
            return result;
        }
    }
    
    size_t space_pos = trimmed.find(' ');
    size_t tab_pos = trimmed.find('\t');
    size_t sep = std::min(space_pos, tab_pos);
    if (sep == std::string::npos) sep = std::max(space_pos, tab_pos);
    
    if (sep != std::string::npos) {
        result.instruction = trimmed.substr(0, sep);
        result.operands = trimmed.substr(sep + 1);
        
        size_t op_start = 0;
        while (op_start < result.operands.size() && 
               (result.operands[op_start] == ' ' || result.operands[op_start] == '\t')) {
            op_start++;
        }
        result.operands = result.operands.substr(op_start);
        
        size_t comment_pos = result.operands.find(';');
        if (comment_pos != std::string::npos) {
            result.comment = result.operands.substr(comment_pos + 1);
            result.operands = result.operands.substr(0, comment_pos);
            while (!result.operands.empty() && 
                   (result.operands.back() == ' ' || result.operands.back() == '\t')) {
                result.operands.pop_back();
            }
            size_t ns = 0;
            while (ns < result.comment.size() && (result.comment[ns] == ' ' || result.comment[ns] == '\t')) {
                ns++;
            }
            if (ns > 0) result.comment = result.comment.substr(ns);
        }
    } else {
        result.instruction = trimmed;
    }
    
    result.has_indent = has_indent;
    return result;
}

std::string PeepholeOptimizer::rebuildCode(const std::vector<AsmLine>& lines) {
    std::ostringstream oss;
    for (const auto& line : lines) {
        if (line.raw.empty() && line.label.empty() && line.instruction.empty() && line.comment.empty()) {
            oss << "\n";
        } else if (line.is_label) {
            oss << line.label;
            if (!line.comment.empty()) {
                oss << " ; " << line.comment;
            }
            oss << "\n";
            
            if (!line.raw.empty() && line.raw[0] != ';') {
                bool is_dup_label = false;
                if (line.raw.find(':') != std::string::npos) {
                    std::string raw_trimmed = line.raw;
                    size_t raw_colon = raw_trimmed.find(':');
                    bool raw_is_label = true;
                    for (size_t i = 0; i < raw_colon; ++i) {
                        if (raw_trimmed[i] == ' ' || raw_trimmed[i] == '\t') {
                            raw_is_label = false;
                            break;
                        }
                    }
                    if (raw_is_label) is_dup_label = true;
                }
                if (!is_dup_label) {
                    oss << "    " << line.raw << "\n";
                }
            } else if (!line.raw.empty()) {
                oss << line.raw << "\n";
            }
        } else if (line.is_directive) {
            oss << line.raw << "\n";
        } else if (!line.instruction.empty()) {
            oss << "    " << line.instruction;
            if (!line.operands.empty()) {
                oss << " " << line.operands;
            }
            if (!line.comment.empty()) {
                oss << " ; " << line.comment;
            }
            oss << "\n";
        } else if (!line.comment.empty()) {
            if (line.has_indent) {
                oss << "    ; " << line.comment << "\n";
            } else {
                oss << "; " << line.comment << "\n";
            }
        } else {
            oss << line.raw << "\n";
        }
    }
    return oss.str();
}

bool PeepholeOptimizer::applyRedundantMoveElimination(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 1 < lines.size()) {
            AsmLine& curr = lines[i];
            AsmLine& next = lines[i + 1];
            
            if (curr.instruction == "mov" && next.instruction == "mov") {
                bool curr_is_reg_to_reg = !curr.operands.empty() && 
                    curr.operands.find('[') == std::string::npos &&
                    curr.operands.find_first_of("0123456789") == std::string::npos;
                bool next_is_reg_to_reg = !next.operands.empty() && 
                    next.operands.find('[') == std::string::npos &&
                    next.operands.find_first_of("0123456789") == std::string::npos;
                
                if (curr_is_reg_to_reg && next_is_reg_to_reg) {
                    std::string curr_dest, curr_src, next_dest, next_src;
                    
                    size_t comma1 = curr.operands.find(',');
                    if (comma1 != std::string::npos) {
                        curr_dest = curr.operands.substr(0, comma1);
                        curr_src = curr.operands.substr(comma1 + 1);
                        curr_dest.erase(0, curr_dest.find_first_not_of(" \t"));
                        curr_dest.erase(curr_dest.find_last_not_of(" \t") + 1);
                        curr_src.erase(0, curr_src.find_first_not_of(" \t"));
                        curr_src.erase(curr_src.find_last_not_of(" \t") + 1);
                    }
                    
                    size_t comma2 = next.operands.find(',');
                    if (comma2 != std::string::npos) {
                        next_dest = next.operands.substr(0, comma2);
                        next_src = next.operands.substr(comma2 + 1);
                        next_dest.erase(0, next_dest.find_first_not_of(" \t"));
                        next_dest.erase(next_dest.find_last_not_of(" \t") + 1);
                        next_src.erase(0, next_src.find_first_not_of(" \t"));
                        next_src.erase(next_src.find_last_not_of(" \t") + 1);
                    }
                    
                    if (curr_dest == next_dest && curr_src == next_src) {
                        result.push_back(curr);
                        i++;
                        stats.redundant_moves_removed++;
                        stats.total_removed++;
                        changed = true;
                        continue;
                    }
                }
            }
        }
        result.push_back(lines[i]);
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

bool PeepholeOptimizer::applyStoreLoadElimination(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 1 < lines.size()) {
            AsmLine& curr = lines[i];
            AsmLine& next = lines[i + 1];
            
            if (curr.instruction == "mov" && next.instruction == "mov") {
                bool curr_is_store = !curr.operands.empty() && 
                    curr.operands[0] == '[' && curr.operands.find(']') != std::string::npos;
                bool next_is_load = !next.operands.empty() && 
                    next.operands.find(", [") != std::string::npos;
                
                if (curr_is_store && next_is_load) {
                    std::string curr_mem, curr_reg, next_reg, next_mem;
                    
                    size_t comma_curr = curr.operands.find(',');
                    if (comma_curr != std::string::npos) {
                        curr_mem = curr.operands.substr(0, comma_curr);
                        curr_reg = curr.operands.substr(comma_curr + 1);
                        curr_mem.erase(0, curr_mem.find_first_not_of(" \t"));
                        curr_mem.erase(curr_mem.find_last_not_of(" \t") + 1);
                        curr_reg.erase(0, curr_reg.find_first_not_of(" \t"));
                        curr_reg.erase(curr_reg.find_last_not_of(" \t") + 1);
                    }
                    
                    size_t comma_next = next.operands.find(',');
                    if (comma_next != std::string::npos) {
                        next_reg = next.operands.substr(0, comma_next);
                        next_mem = next.operands.substr(comma_next + 1);
                        next_reg.erase(0, next_reg.find_first_not_of(" \t"));
                        next_reg.erase(next_reg.find_last_not_of(" \t") + 1);
                        next_mem.erase(0, next_mem.find_first_not_of(" \t"));
                        next_mem.erase(next_mem.find_last_not_of(" \t") + 1);
                    }
                    
                    if (curr_mem == next_mem && curr_reg == next_reg) {
                        result.push_back(curr);
                        i++;
                        stats.redundant_loads_removed++;
                        stats.total_removed++;
                        changed = true;
                        continue;
                    }
                }
            }
        }
        result.push_back(lines[i]);
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

bool PeepholeOptimizer::applyIncDecReplacement(std::vector<AsmLine>& lines) {
    bool changed = false;
    
    for (auto& line : lines) {
        if (!line.is_label && !line.is_directive) {
            if (line.instruction == "add" && !line.operands.empty()) {
                size_t comma = line.operands.find(',');
                if (comma != std::string::npos) {
                    std::string op2 = line.operands.substr(comma + 1);
                    op2.erase(0, op2.find_first_not_of(" \t"));
                    op2.erase(op2.find_last_not_of(" \t") + 1);
                    
                    if (op2 == "1") {
                        std::string dest = line.operands.substr(0, comma);
                        dest.erase(dest.find_last_not_of(" \t") + 1);
                        line.instruction = "inc";
                        line.operands = dest;
                        stats.inc_dec_replacements++;
                        stats.total_removed++;
                        changed = true;
                    }
                }
            } else if (line.instruction == "sub" && !line.operands.empty()) {
                size_t comma = line.operands.find(',');
                if (comma != std::string::npos) {
                    std::string op2 = line.operands.substr(comma + 1);
                    op2.erase(0, op2.find_first_not_of(" \t"));
                    op2.erase(op2.find_last_not_of(" \t") + 1);
                    
                    if (op2 == "1") {
                        std::string dest = line.operands.substr(0, comma);
                        dest.erase(dest.find_last_not_of(" \t") + 1);
                        line.instruction = "dec";
                        line.operands = dest;
                        stats.inc_dec_replacements++;
                        stats.total_removed++;
                        changed = true;
                    }
                }
            }
        }
    }
    
    return changed;
}

bool PeepholeOptimizer::applyXorZeroReplacement(std::vector<AsmLine>& lines) {
    bool changed = false;
    
    for (auto& line : lines) {
        if (!line.is_label && !line.is_directive) {
            if (line.instruction == "mov" && !line.operands.empty()) {
                size_t comma = line.operands.find(',');
                if (comma != std::string::npos) {
                    std::string dest = line.operands.substr(0, comma);
                    std::string src = line.operands.substr(comma + 1);
                    
                    dest.erase(0, dest.find_first_not_of(" \t"));
                    dest.erase(dest.find_last_not_of(" \t") + 1);
                    src.erase(0, src.find_first_not_of(" \t"));
                    src.erase(src.find_last_not_of(" \t") + 1);
                    
                    if (src == "0" && dest.find('[') == std::string::npos) {
                        line.instruction = "xor";
                        line.operands = dest + ", " + dest;
                        stats.xor_zero_replacements++;
                        stats.total_removed++;
                        changed = true;
                    }
                }
            }
        }
    }
    
    return changed;
}

bool PeepholeOptimizer::applyTestBeforeJumpElimination(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    auto setsZeroFlag = [](const std::string& instr) -> bool {
        return instr == "add" || instr == "sub" || instr == "and" || 
               instr == "or" || instr == "xor" || instr == "cmp" ||
               instr == "inc" || instr == "dec" || instr == "neg" ||
               instr == "shl" || instr == "shr" || instr == "imul" ||
               instr == "not";
    };
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 2 < lines.size()) {
            AsmLine& prev = lines[i];
            AsmLine& test_instr = lines[i + 1];
            AsmLine& jump = lines[i + 2];
            
            if (test_instr.instruction == "test" && !test_instr.operands.empty()) {
                size_t comma = test_instr.operands.find(',');
                if (comma != std::string::npos) {
                    std::string op1 = test_instr.operands.substr(0, comma);
                    std::string op2 = test_instr.operands.substr(comma + 1);
                    op1.erase(0, op1.find_first_not_of(" \t"));
                    op1.erase(op1.find_last_not_of(" \t") + 1);
                    op2.erase(0, op2.find_first_not_of(" \t"));
                    op2.erase(op2.find_last_not_of(" \t") + 1);
                    
                    if (op1 == op2 && op1.find('[') == std::string::npos) {
                        bool prev_sets_zf = setsZeroFlag(prev.instruction);
                        
                        bool prev_uses_same_reg = false;
                        if (!prev.operands.empty()) {
                            std::string prev_dest;
                            size_t prev_comma = prev.operands.find(',');
                            if (prev_comma != std::string::npos) {
                                prev_dest = prev.operands.substr(0, prev_comma);
                            } else {
                                prev_dest = prev.operands;
                            }
                            prev_dest.erase(0, prev_dest.find_first_not_of(" \t"));
                            prev_dest.erase(prev_dest.find_last_not_of(" \t") + 1);
                            
                            if (prev_dest == op1) {
                                prev_uses_same_reg = true;
                            }
                        }
                        
                        if ((prev_sets_zf && prev_uses_same_reg) || prev.instruction == "cmp") {
                            if (jump.instruction == "jz" || jump.instruction == "jnz" ||
                                jump.instruction == "je" || jump.instruction == "jne") {
                                result.push_back(prev);
                                result.push_back(jump);
                                i += 2;
                                stats.test_removed++;
                                stats.total_removed++;
                                changed = true;
                                continue;
                            }
                        }
                    }
                }
            }
        }
        result.push_back(lines[i]);
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

bool PeepholeOptimizer::applySelfMoveElimination(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        AsmLine& line = lines[i];
        bool skip = false;
        
        if (!line.is_label && !line.is_directive && line.instruction == "mov" && !line.operands.empty()) {
            size_t comma = line.operands.find(',');
            if (comma != std::string::npos) {
                std::string dest = line.operands.substr(0, comma);
                std::string src = line.operands.substr(comma + 1);
                
                dest.erase(0, dest.find_first_not_of(" \t"));
                dest.erase(dest.find_last_not_of(" \t") + 1);
                src.erase(0, src.find_first_not_of(" \t"));
                src.erase(src.find_last_not_of(" \t") + 1);
                
                if (dest == src && dest.find('[') == std::string::npos) {
                    skip = true;
                    stats.self_moves_removed++;
                    stats.total_removed++;
                    changed = true;
                }
            }
        }
        
        if (!skip) {
            result.push_back(line);
        }
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

bool PeepholeOptimizer::applySwapMoveElimination(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 1 < lines.size()) {
            AsmLine& curr = lines[i];
            AsmLine& next = lines[i + 1];
            
            if (curr.instruction == "mov" && next.instruction == "mov" &&
                !curr.operands.empty() && !next.operands.empty() &&
                !curr.is_label && !next.is_label) {
                
                bool curr_is_reg = curr.operands.find('[') == std::string::npos;
                bool next_is_reg = next.operands.find('[') == std::string::npos;
                
                if (curr_is_reg && next_is_reg) {
                    std::string curr_dest, curr_src, next_dest, next_src;
                    
                    size_t comma1 = curr.operands.find(',');
                    if (comma1 != std::string::npos) {
                        curr_dest = curr.operands.substr(0, comma1);
                        curr_src = curr.operands.substr(comma1 + 1);
                        curr_dest.erase(0, curr_dest.find_first_not_of(" \t"));
                        curr_dest.erase(curr_dest.find_last_not_of(" \t") + 1);
                        curr_src.erase(0, curr_src.find_first_not_of(" \t"));
                        curr_src.erase(curr_src.find_last_not_of(" \t") + 1);
                    }
                    
                    size_t comma2 = next.operands.find(',');
                    if (comma2 != std::string::npos) {
                        next_dest = next.operands.substr(0, comma2);
                        next_src = next.operands.substr(comma2 + 1);
                        next_dest.erase(0, next_dest.find_first_not_of(" \t"));
                        next_dest.erase(next_dest.find_last_not_of(" \t") + 1);
                        next_src.erase(0, next_src.find_first_not_of(" \t"));
                        next_src.erase(next_src.find_last_not_of(" \t") + 1);
                    }
                    
                    if (curr_dest == next_src && curr_src == next_dest && 
                        curr_dest != curr_src) {
                        
                        bool safe = true;
                        if (next_dest == "eax" && i + 2 < lines.size()) {
                            AsmLine& after = lines[i + 2];
                            if (after.instruction == "ret") safe = false;
                        }
                        if (safe) {
                            result.push_back(curr);
                            i++;
                            stats.redundant_moves_removed++;
                            stats.total_removed++;
                            changed = true;
                            continue;
                        }
                    }
                }
            }
        }
        result.push_back(lines[i]);
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

bool PeepholeOptimizer::applyShiftReplacement(std::vector<AsmLine>& lines) {
    bool changed = false;
    
    for (auto& line : lines) {
        if (!line.is_label && !line.is_directive) {
            if (line.instruction == "imul" && !line.operands.empty()) {
                size_t comma = line.operands.find(',');
                if (comma != std::string::npos) {
                    std::string dest = line.operands.substr(0, comma);
                    std::string src = line.operands.substr(comma + 1);
                    
                    dest.erase(0, dest.find_first_not_of(" \t"));
                    dest.erase(dest.find_last_not_of(" \t") + 1);
                    src.erase(0, src.find_first_not_of(" \t"));
                    src.erase(src.find_last_not_of(" \t") + 1);
                    
                    if (src.find_first_not_of("0123456789") == std::string::npos) {
                        int value = std::stoi(src);
                        if ((value & (value - 1)) == 0 && value > 0) {
                            int shift = 0;
                            while (value > 1) {
                                value >>= 1;
                                shift++;
                            }
                            line.instruction = "shl";
                            line.operands = dest + ", " + std::to_string(shift);
                            stats.mul_to_shift_replacements++;
                            stats.total_removed++;
                            changed = true;
                        }
                    }
                }
            }
        }
    }
    
    return changed;
}

bool PeepholeOptimizer::applyUselessJumpRemoval(std::vector<AsmLine>& lines) {
    bool changed = false;
    std::vector<AsmLine> result;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        if (i + 1 < lines.size()) {
            AsmLine& curr = lines[i];
            AsmLine& next = lines[i + 1];
            
            if (curr.instruction == "jmp" && next.is_label) {
                std::string target = curr.operands;
                target.erase(0, target.find_first_not_of(" \t"));
                target.erase(target.find_last_not_of(" \t") + 1);
                
                std::string label = next.label;
                if (!label.empty() && label.back() == ':') {
                    label.pop_back();
                }
                
                if (target == label) {
                    stats.useless_jumps_removed++;
                    stats.total_removed++;
                    changed = true;
                    continue;
                }
            }
        }
        result.push_back(lines[i]);
    }
    
    if (changed) {
        lines = std::move(result);
    }
    return changed;
}

std::string PeepholeOptimizer::optimize(const std::string& asm_code) {
    resetStats();
    
    std::vector<AsmLine> lines;
    std::istringstream iss(asm_code);
    std::string line_str;
    
    while (std::getline(iss, line_str)) {
        if (!line_str.empty() && line_str.back() == '\r') {
            line_str.pop_back();
        }
        lines.push_back(parseLine(line_str));
    }
    
    bool changed = true;
    int max_iterations = 10;
    int iteration = 0;
    
    while (changed && iteration < max_iterations) {
        changed = false;
        
        if (applySelfMoveElimination(lines)) changed = true;
        if (applySwapMoveElimination(lines)) changed = true;
        if (applyRedundantMoveElimination(lines)) changed = true;
        if (applyStoreLoadElimination(lines)) changed = true;
        if (applyIncDecReplacement(lines)) changed = true;
        if (applyXorZeroReplacement(lines)) changed = true;
        if (applyTestBeforeJumpElimination(lines)) changed = true;
        if (applyShiftReplacement(lines)) changed = true;
        if (applyUselessJumpRemoval(lines)) changed = true;
        
        iteration++;
    }
    
    return rebuildCode(lines);
}

}