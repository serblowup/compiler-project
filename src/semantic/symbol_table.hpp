#ifndef SEMANTIC_SYMBOL_TABLE_HPP
#define SEMANTIC_SYMBOL_TABLE_HPP

#include "symbol.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <iomanip>

namespace semantic {

       /* Области видимости:
       - глобальная
       - функции
       - блока
       - структуры
       - then ветки if
       - else ветки if
       - цикла for
       - тело цикла for
       - тело цикла while
    */
    enum class ScopeType {
        GLOBAL,
        FUNCTION,
        BLOCK,
        STRUCT,
        IF_THEN,
        IF_ELSE,
        FOR_LOOP,
        FOR_BODY,
        WHILE_BODY
    };

    inline std::string scopeTypeToString(ScopeType type) {
        switch (type) {
            case ScopeType::GLOBAL: return "global";
            case ScopeType::FUNCTION: return "function";
            case ScopeType::BLOCK: return "block";
            case ScopeType::STRUCT: return "struct";
            case ScopeType::IF_THEN: return "if-then";
            case ScopeType::IF_ELSE: return "if-else";
            case ScopeType::FOR_LOOP: return "for-loop";
            case ScopeType::FOR_BODY: return "for-body";
            case ScopeType::WHILE_BODY: return "while-body";
            default: return "unknown";
        }
    }

    // Информация об области видимости
    struct ScopeInfo {
        ScopeType type;
        std::string name;
        int depth;
        int index;
        int parentIndex;
        
        ScopeInfo(ScopeType t, const std::string& n = "", int d = 0, int idx = 0, int parentIdx = -1)
            : type(t), name(n), depth(d), index(idx), parentIndex(parentIdx) {}
        
        std::string toString() const {
            std::string result = scopeTypeToString(type);
            if (!name.empty()) {
                result += ":" + name;
            }
            return result;
        }
    };

    // Таблица символов
    class SymbolTable {
    private:
        std::vector<std::unordered_map<std::string, SymbolInfo>> scopes;
        std::vector<ScopeInfo> scopeInfos;
        std::vector<int> activeScopeIndices;
        
        std::vector<std::unordered_map<std::string, SymbolInfo>> allScopes;
        std::vector<ScopeInfo> allScopeInfos;
        
        int nextScopeIndex;

    public:
        SymbolTable() : nextScopeIndex(0) {
            enterScope(ScopeType::GLOBAL, "global");
        }

        SymbolTable(const SymbolTable& other)
        : scopes(other.scopes),
          scopeInfos(other.scopeInfos),
          activeScopeIndices(other.activeScopeIndices),
          allScopes(other.allScopes),
          allScopeInfos(other.allScopeInfos),
          nextScopeIndex(other.nextScopeIndex) {}

        void enterScope(ScopeType type, const std::string& name = "") {
            int parentIndex = scopeInfos.empty() ? -1 : scopeInfos.back().index;
            int currentIndex = nextScopeIndex++;
            int depth = scopeInfos.size();
            
            int allScopesIndex = allScopes.size();
            allScopes.emplace_back();
            allScopeInfos.emplace_back(type, name, depth, currentIndex, parentIndex);
            
            scopes.emplace_back();
            scopeInfos.emplace_back(type, name, depth, currentIndex, parentIndex);
            activeScopeIndices.push_back(allScopesIndex);
        }

        void exitScope() {
            if (!scopes.empty()) {
                int activeIndex = activeScopeIndices.back();
                if (activeIndex >= 0 && activeIndex < static_cast<int>(allScopes.size())) {
                    allScopes[activeIndex] = scopes.back();
                }
                scopes.pop_back();
                scopeInfos.pop_back();
                activeScopeIndices.pop_back();
            }
        }

        bool insert(const std::string& name, const SymbolInfo& info) {
            if (scopes.empty()) {
                return false;
            }

            auto& currentScope = scopes.back();
            if (currentScope.find(name) != currentScope.end()) {
                return false;
            }

            currentScope[name] = info;
            
            int activeIndex = activeScopeIndices.back();
            if (activeIndex >= 0 && activeIndex < static_cast<int>(allScopes.size())) {
                allScopes[activeIndex][name] = info;
            }

            return true;
        }

        SymbolInfo* lookup(const std::string& name) {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                auto found = it->find(name);
                if (found != it->end()) {
                    return &found->second;
                }
            }
            return nullptr;
        }

        SymbolInfo* lookupCurrent(const std::string& name) {
            if (scopes.empty()) {
                return nullptr;
            }

            auto& currentScope = scopes.back();
            auto found = currentScope.find(name);
            if (found != currentScope.end()) {
                return &found->second;
            }
            return nullptr;
        }

        bool exists(const std::string& name) const {
            for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
                if (it->find(name) != it->end()) {
                    return true;
                }
            }
            return false;
        }

        bool existsCurrent(const std::string& name) const {
            if (scopes.empty()) {
                return false;
            }
            return scopes.back().find(name) != scopes.back().end();
        }

        int getDepth() const {
            return scopeInfos.size();
        }

        ScopeInfo getCurrentScopeInfo() const {
            if (scopeInfos.empty()) {
                return ScopeInfo(ScopeType::GLOBAL, "", 0, -1, -1);
            }
            return scopeInfos.back();
        }
        
        std::string getCurrentScopeName() const {
            if (scopeInfos.empty()) {
                return "global";
            }
            return scopeInfos.back().toString();
        }

        ScopeType getCurrentScopeType() const {
            if (scopeInfos.empty()) {
                return ScopeType::GLOBAL;
            }
            return scopeInfos.back().type;
        }

        std::vector<SymbolInfo> getCurrentScopeSymbols() const {
            std::vector<SymbolInfo> result;
            if (!scopes.empty()) {
                for (const auto& pair : scopes.back()) {
                    result.push_back(pair.second);
                }
            }
            return result;
        }

        std::vector<SymbolInfo> getGlobalSymbols() const {
            std::vector<SymbolInfo> result;
            if (!allScopes.empty()) {
                for (const auto& pair : allScopes[0]) {
                    result.push_back(pair.second);
                }
            }
            return result;
        }

        SymbolTable getSnapshot() const {
            SymbolTable copy;
            copy.allScopes = this->allScopes;
            copy.allScopeInfos = this->allScopeInfos;
            return copy;
        }

        void clear() {
            scopes.clear();
            scopeInfos.clear();
            activeScopeIndices.clear();
            allScopes.clear();
            allScopeInfos.clear();
            nextScopeIndex = 0;
            enterScope(ScopeType::GLOBAL, "global");
        }

        void printScopeHierarchy(std::ostringstream& oss, int scopeIndex, int indentLevel) const {
            if (scopeIndex >= static_cast<int>(allScopeInfos.size())) {
                return;
            }
            
            const auto& scopeInfo = allScopeInfos[scopeIndex];
            std::string indent(indentLevel * 2, ' ');
            
            oss << indent << "Scope " << scopeIndex << " [" << scopeInfo.toString() << "]";
            if (scopeInfo.parentIndex >= 0) {
                oss << " (parent: " << allScopeInfos[scopeInfo.parentIndex].toString() << ")";
            }
            oss << ":\n";
            
            if (allScopes[scopeIndex].empty()) {
                oss << indent << "  (no symbols)\n";
            } else {
                for (const auto& pair : allScopes[scopeIndex]) {
                    oss << indent << "  " << pair.second.toString() << "\n";
                }
            }
            
            for (size_t i = scopeIndex + 1; i < allScopeInfos.size(); ++i) {
                if (allScopeInfos[i].parentIndex == scopeInfo.index) {
                    printScopeHierarchy(oss, i, indentLevel + 1);
                }
            }
        }

        std::string toString() const {
            std::ostringstream oss;
            oss << "Symbol Table:\n";
            
            for (size_t i = 0; i < allScopeInfos.size(); ++i) {
                if (allScopeInfos[i].parentIndex == -1) {
                    printScopeHierarchy(oss, i, 0);
                }
            }
            
            return oss.str();
        }

        std::string toJSON() const {
            std::ostringstream oss;
            oss << "{\n";
            oss << "  \"symbol_table\": {\n";
            oss << "    \"scopes\": [\n";

            for (size_t i = 0; i < allScopes.size(); ++i) {
                const auto& scopeInfo = allScopeInfos[i];
                
                oss << "      {\n";
                oss << "        \"index\": " << i << ",\n";
                oss << "        \"name\": \"" << escapeJSON(scopeInfo.toString()) << "\",\n";
                oss << "        \"type\": \"" << scopeTypeToString(scopeInfo.type) << "\",\n";
                oss << "        \"depth\": " << scopeInfo.depth << ",\n";
                oss << "        \"parent_index\": " << scopeInfo.parentIndex << ",\n";
                oss << "        \"symbols\": [\n";

                size_t symCount = 0;
                for (const auto& pair : allScopes[i]) {
                    oss << "          {\n";
                    oss << "            \"name\": \"" << escapeJSON(pair.second.name) << "\",\n";
                    oss << "            \"type\": \"" << escapeJSON(pair.second.type ? pair.second.type->toString() : "unknown") << "\",\n";
                    oss << "            \"kind\": \"" << symbolKindToString(pair.second.kind) << "\",\n";
                    oss << "            \"line\": " << pair.second.line << ",\n";
                    oss << "            \"column\": " << pair.second.column;

                    if (pair.second.isFunction()) {
                        oss << ",\n            \"return_type\": \"" << escapeJSON(pair.second.returnType ? pair.second.returnType->toString() : "void") << "\",\n";
                        oss << "            \"parameters\": [\n";
                        for (size_t j = 0; j < pair.second.parameters.size(); ++j) {
                            oss << "              {\n";
                            oss << "                \"name\": \"" << escapeJSON(pair.second.parameters[j].name) << "\",\n";
                            oss << "                \"type\": \"" << escapeJSON(pair.second.parameters[j].type->toString()) << "\"\n";
                            oss << "              }";
                            if (j < pair.second.parameters.size() - 1) oss << ",";
                            oss << "\n";
                        }
                        oss << "            ]\n";
                    } else if (pair.second.isStruct() && !pair.second.fields.empty()) {
                        oss << ",\n            \"fields\": [\n";
                        for (size_t j = 0; j < pair.second.fields.size(); ++j) {
                            oss << "              {\n";
                            oss << "                \"name\": \"" << escapeJSON(pair.second.fields[j]->name) << "\",\n";
                            oss << "                \"type\": \"" << escapeJSON(pair.second.fields[j]->type ? pair.second.fields[j]->type->toString() : "unknown") << "\"\n";
                            oss << "              }";
                            if (j < pair.second.fields.size() - 1) oss << ",";
                            oss << "\n";
                        }
                        oss << "            ]\n";
                    } else {
                        oss << "\n";
                    }

                    oss << "          }";
                    if (symCount < allScopes[i].size() - 1) oss << ",";
                    oss << "\n";
                    symCount++;
                }

                oss << "        ]\n";
                oss << "      }";
                if (i < allScopes.size() - 1) oss << ",";
                oss << "\n";
            }

            oss << "    ]\n";
            oss << "  }\n";
            oss << "}\n";

            return oss.str();
        }

    private:
        std::string escapeJSON(const std::string& str) const {
            std::ostringstream oss;
            for (char c : str) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: oss << c;
                }
            }
            return oss.str();
        }
    };

}

#endif