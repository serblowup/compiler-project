#include "label_manager.hpp"
#include <sstream>

namespace codegen {

LabelManager::LabelManager() 
    : label_counter(0)
    , temp_counter(0) {
}

std::string LabelManager::newLabel() {
    return "L" + std::to_string(++label_counter);
}

std::string LabelManager::newLabel(const std::string& prefix) {
    return prefix + "_" + std::to_string(++label_counter);
}

std::string LabelManager::newTemp() {
    return "t" + std::to_string(++temp_counter);
}

void LabelManager::pushLoop(const std::string& break_label, const std::string& continue_label) {
    loop_stack.push({break_label, continue_label});
}

void LabelManager::popLoop() {
    if (!loop_stack.empty()) {
        loop_stack.pop();
    }
}

std::string LabelManager::getCurrentBreakLabel() const {
    return loop_stack.empty() ? "" : loop_stack.top().break_label;
}

std::string LabelManager::getCurrentContinueLabel() const {
    return loop_stack.empty() ? "" : loop_stack.top().continue_label;
}

bool LabelManager::isInLoop() const {
    return !loop_stack.empty();
}

void LabelManager::mapLabel(const std::string& from, const std::string& to) {
    label_map[from] = to;
}

std::string LabelManager::getMappedLabel(const std::string& label) const {
    auto it = label_map.find(label);
    return it != label_map.end() ? it->second : label;
}

void LabelManager::reset() {
    label_counter = 0;
    temp_counter = 0;
    while (!loop_stack.empty()) loop_stack.pop();
    label_map.clear();
}

}