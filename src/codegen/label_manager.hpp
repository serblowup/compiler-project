#ifndef CODEGEN_LABEL_MANAGER_HPP
#define CODEGEN_LABEL_MANAGER_HPP

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>

namespace codegen {

struct LoopContext {
    std::string break_label;
    std::string continue_label;
};

class LabelManager {
private:
    int label_counter;
    int temp_counter;
    std::stack<LoopContext> loop_stack;
    std::unordered_map<std::string, std::string> label_map;
    
public:
    LabelManager();
    
    // Генерация новых меток
    std::string newLabel();
    std::string newLabel(const std::string& prefix);
    std::string newTemp();
    
    // Управление циклами (для break/continue)
    void pushLoop(const std::string& break_label, const std::string& continue_label);
    void popLoop();
    std::string getCurrentBreakLabel() const;
    std::string getCurrentContinueLabel() const;
    bool isInLoop() const;
    
    // Маппинг меток (для Phi-функций)
    void mapLabel(const std::string& from, const std::string& to);
    std::string getMappedLabel(const std::string& label) const;
    
    void reset();
    
    // Статистика
    int getLabelCount() const { return label_counter; }
    int getTempCount() const { return temp_counter; }
};

}

#endif