#include "ast.hpp"
#include <sstream>
#include <iomanip>

std::string ProgramNode::toString() const {
    std::ostringstream oss;
    oss << "Program [line " << line << "]:\n";
    for (const auto& decl : declarations) {
        std::string declStr = decl->toString();
        std::istringstream iss(declStr);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "  " << line << "\n";
        }
    }
    return oss.str();
}

std::string LiteralExprNode::toString() const {
    std::ostringstream oss;
    oss << "Literal [line " << line << "]: ";
    
    switch (type) {
        case TokenType::tkn_INT_LITERAL:
            if (auto val = getValue<int>()) {
                oss << "int " << *val;
            }
            break;
        case TokenType::tkn_FLOAT_LITERAL:
            if (auto val = getValue<double>()) {
                oss << "float " << std::fixed << std::setprecision(6) << *val;
            }
            break;
        case TokenType::tkn_STRING_LITERAL:
            if (auto val = getValue<std::string>()) {
                oss << "string \"" << *val << "\"";
            }
            break;
        case TokenType::tkn_BOOLEAN_LITERAL:
            if (auto val = getValue<bool>()) {
                oss << "bool " << (*val ? "true" : "false");
            }
            break;
        default:
            oss << "unknown";
    }
    
    return oss.str();
}

std::string IdentifierExprNode::toString() const {
    std::ostringstream oss;
    oss << "Identifier [line " << line << "]: " << name;
    return oss.str();
}

std::string BinaryExprNode::toString() const {
    std::ostringstream oss;
    oss << "Binary [line " << line << "]: " << token_type_to_string(op) << "\n";
    
    std::string leftStr = left->toString();
    std::istringstream leftIss(leftStr);
    std::string leftLine;
    while (std::getline(leftIss, leftLine)) {
        oss << "  left: " << leftLine << "\n";
    }
    
    std::string rightStr = right->toString();
    std::istringstream rightIss(rightStr);
    std::string rightLine;
    while (std::getline(rightIss, rightLine)) {
        oss << "  right: " << rightLine << "\n";
    }
    
    return oss.str();
}

std::string UnaryExprNode::toString() const {
    std::ostringstream oss;
    oss << "Unary [line " << line << "]: " << token_type_to_string(op) << "\n";
    
    std::string operandStr = operand->toString();
    std::istringstream iss(operandStr);
    std::string line;
    while (std::getline(iss, line)) {
        oss << "  " << line << "\n";
    }
    
    return oss.str();
}

std::string CallExprNode::toString() const {
    std::ostringstream oss;
    oss << "Call [line " << line << "]:\n";
    
    std::string calleeStr = callee->toString();
    std::istringstream calleeIss(calleeStr);
    std::string calleeLine;
    while (std::getline(calleeIss, calleeLine)) {
        oss << "  callee: " << calleeLine << "\n";
    }
    
    if (!arguments.empty()) {
        oss << "  arguments:\n";
        for (const auto& arg : arguments) {
            std::string argStr = arg->toString();
            std::istringstream argIss(argStr);
            std::string argLine;
            while (std::getline(argIss, argLine)) {
                oss << "    " << argLine << "\n";
            }
        }
    }
    
    return oss.str();
}

std::string AssignmentExprNode::toString() const {
    std::ostringstream oss;
    oss << "Assignment [line " << line << "]: " << token_type_to_string(op) << "\n";
    
    std::string targetStr = target->toString();
    std::istringstream targetIss(targetStr);
    std::string targetLine;
    while (std::getline(targetIss, targetLine)) {
        oss << "  target: " << targetLine << "\n";
    }
    
    std::string valueStr = value->toString();
    std::istringstream valueIss(valueStr);
    std::string valueLine;
    while (std::getline(valueIss, valueLine)) {
        oss << "  value: " << valueLine << "\n";
    }
    
    return oss.str();
}

std::string BlockStmtNode::toString() const {
    std::ostringstream oss;
    oss << "Block [line " << line << "]:\n";
    
    for (const auto& stmt : statements) {
        std::string stmtStr = stmt->toString();
        std::istringstream iss(stmtStr);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "  " << line << "\n";
        }
    }
    
    return oss.str();
}

std::string ExprStmtNode::toString() const {
    std::ostringstream oss;
    oss << "ExprStmt [line " << line << "]:\n";
    
    std::string exprStr = expression->toString();
    std::istringstream iss(exprStr);
    std::string line;
    while (std::getline(iss, line)) {
        oss << "  " << line << "\n";
    }
    
    return oss.str();
}

std::string IfStmtNode::toString() const {
    std::ostringstream oss;
    oss << "IfStmt [line " << line << "]:\n";
    
    std::string condStr = condition->toString();
    std::istringstream condIss(condStr);
    std::string condLine;
    while (std::getline(condIss, condLine)) {
        oss << "  condition: " << condLine << "\n";
    }
    
    oss << "  then:\n";
    std::string thenStr = thenBranch->toString();
    std::istringstream thenIss(thenStr);
    std::string thenLine;
    while (std::getline(thenIss, thenLine)) {
        oss << "    " << thenLine << "\n";
    }
    
    if (elseBranch) {
        oss << "  else:\n";
        std::string elseStr = elseBranch->toString();
        std::istringstream elseIss(elseStr);
        std::string elseLine;
        while (std::getline(elseIss, elseLine)) {
            oss << "    " << elseLine << "\n";
        }
    }
    
    return oss.str();
}

std::string WhileStmtNode::toString() const {
    std::ostringstream oss;
    oss << "WhileStmt [line " << line << "]:\n";
    
    std::string condStr = condition->toString();
    std::istringstream condIss(condStr);
    std::string condLine;
    while (std::getline(condIss, condLine)) {
        oss << "  condition: " << condLine << "\n";
    }
    
    oss << "  body:\n";
    std::string bodyStr = body->toString();
    std::istringstream bodyIss(bodyStr);
    std::string bodyLine;
    while (std::getline(bodyIss, bodyLine)) {
        oss << "    " << bodyLine << "\n";
    }
    
    return oss.str();
}

std::string ForStmtNode::toString() const {
    std::ostringstream oss;
    oss << "ForStmt [line " << line << "]:\n";
    
    if (init) {
        oss << "  init:\n";
        std::string initStr = init->toString();
        std::istringstream initIss(initStr);
        std::string initLine;
        while (std::getline(initIss, initLine)) {
            oss << "    " << initLine << "\n";
        }
    }
    
    if (condition) {
        std::string condStr = condition->toString();
        std::istringstream condIss(condStr);
        std::string condLine;
        while (std::getline(condIss, condLine)) {
            oss << "  condition: " << condLine << "\n";
        }
    }
    
    if (update) {
        std::string updateStr = update->toString();
        std::istringstream updateIss(updateStr);
        std::string updateLine;
        while (std::getline(updateIss, updateLine)) {
            oss << "  update: " << updateLine << "\n";
        }
    }
    
    oss << "  body:\n";
    std::string bodyStr = body->toString();
    std::istringstream bodyIss(bodyStr);
    std::string bodyLine;
    while (std::getline(bodyIss, bodyLine)) {
        oss << "    " << bodyLine << "\n";
    }
    
    return oss.str();
}

std::string ReturnStmtNode::toString() const {
    std::ostringstream oss;
    oss << "ReturnStmt [line " << line << "]:";
    
    if (value) {
        oss << "\n";
        std::string valueStr = value->toString();
        std::istringstream iss(valueStr);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "  " << line << "\n";
        }
    } else {
        oss << " void\n";
    }
    
    return oss.str();
}

std::string VarDeclStmtNode::toString() const {
    std::ostringstream oss;
    oss << "VarDecl [line " << line << "]: " << type << " " << name;
    
    if (initializer) {
        oss << " =\n";
        std::string initStr = initializer->toString();
        std::istringstream iss(initStr);
        std::string line;
        while (std::getline(iss, line)) {
            oss << "  " << line << "\n";
        }
    } else {
        oss << "\n";
    }
    
    return oss.str();
}

std::string ParamNode::toString() const {
    std::ostringstream oss;
    oss << "Param [line " << line << "]: " << type << " " << name;
    return oss.str();
}

std::string FunctionDeclNode::toString() const {
    std::ostringstream oss;
    oss << "FunctionDecl [line " << line << "]: " << name << " -> " << returnType << "\n";
    
    if (!parameters.empty()) {
        oss << "  parameters:\n";
        for (const auto& param : parameters) {
            std::string paramStr = param->toString();
            std::istringstream iss(paramStr);
            std::string line;
            while (std::getline(iss, line)) {
                oss << "    " << line << "\n";
            }
        }
    }
    
    oss << "  body:\n";
    std::string bodyStr = body->toString();
    std::istringstream bodyIss(bodyStr);
    std::string bodyLine;
    while (std::getline(bodyIss, bodyLine)) {
        oss << "    " << bodyLine << "\n";
    }
    
    return oss.str();
}

std::string StructDeclNode::toString() const {
    std::ostringstream oss;
    oss << "StructDecl [line " << line << "]: " << name << "\n";
    
    if (!fields.empty()) {
        oss << "  fields:\n";
        for (const auto& field : fields) {
            std::string fieldStr = field->toString();
            std::istringstream iss(fieldStr);
            std::string line;
            while (std::getline(iss, line)) {
                oss << "    " << line << "\n";
            }
        }
    }
    
    return oss.str();
}
