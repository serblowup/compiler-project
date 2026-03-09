#include "ast_visualizer.hpp"
#include <iomanip>
#include <stack>
#include <map>

// Pretty Printer
std::string PrettyPrinter::visualize(const ProgramNode* program) {
    if (!program) return "";
    return program->toString();
}

// DOT
std::string DOTPrinter::generateNodeId() {
    return "node" + std::to_string(nodeCounter++);
}

void DOTPrinter::visitNode(const ASTNode* node, std::ostream& out) {
    if (!node) return;
    
    std::string nodeId = generateNodeId();
    
    std::string color = "lightblue";
    std::string label;
    std::string shape = "box";
    
    if (dynamic_cast<const ProgramNode*>(node)) {
        color = "lightgreen";
        label = "Program";
        shape = "box";
    } else if (dynamic_cast<const FunctionDeclNode*>(node)) {
        color = "lightcoral";
        auto func = dynamic_cast<const FunctionDeclNode*>(node);
        label = "Function: " + func->getName() + "\\n-> " + func->getReturnType();
        shape = "box";
    } else if (dynamic_cast<const StructDeclNode*>(node)) {
        color = "lightgoldenrodyellow";
        auto str = dynamic_cast<const StructDeclNode*>(node);
        label = "Struct: " + str->getName();
        shape = "box";
    } else if (dynamic_cast<const VarDeclStmtNode*>(node)) {
        color = "lightskyblue";
        auto var = dynamic_cast<const VarDeclStmtNode*>(node);
        label = "VarDecl: " + var->getType() + " " + var->getName();
        if (var->hasInitializer()) {
            label += " = ...";
        }
        shape = "box";
    } else if (dynamic_cast<const BinaryExprNode*>(node)) {
        color = "thistle";
        auto bin = dynamic_cast<const BinaryExprNode*>(node);
        label = "Binary: " + std::string(token_type_to_string(bin->getOperator()));
        shape = "ellipse";
    } else if (dynamic_cast<const UnaryExprNode*>(node)) {
        color = "thistle";
        auto un = dynamic_cast<const UnaryExprNode*>(node);
        label = "Unary: " + std::string(token_type_to_string(un->getOperator()));
        shape = "ellipse";
    } else if (dynamic_cast<const LiteralExprNode*>(node)) {
        color = "wheat";
        auto lit = dynamic_cast<const LiteralExprNode*>(node);
        label = "Literal: " + lit->toString();
        shape = "ellipse";
    } else if (dynamic_cast<const IdentifierExprNode*>(node)) {
        color = "wheat";
        auto id = dynamic_cast<const IdentifierExprNode*>(node);
        label = "Identifier: " + id->getName();
        shape = "ellipse";
    } else if (dynamic_cast<const CallExprNode*>(node)) {
        color = "thistle";
        label = "Call";
        shape = "ellipse";
    } else if (dynamic_cast<const AssignmentExprNode*>(node)) {
        color = "thistle";
        auto assign = dynamic_cast<const AssignmentExprNode*>(node);
        label = "Assignment: " + std::string(token_type_to_string(assign->getOperator()));
        shape = "ellipse";
    } else if (dynamic_cast<const IfStmtNode*>(node)) {
        color = "lightpink";
        label = "If";
        shape = "box";
    } else if (dynamic_cast<const WhileStmtNode*>(node)) {
        color = "lightpink";
        label = "While";
        shape = "box";
    } else if (dynamic_cast<const ForStmtNode*>(node)) {
        color = "lightpink";
        label = "For";
        shape = "box";
    } else if (dynamic_cast<const ReturnStmtNode*>(node)) {
        color = "lightpink";
        label = "Return";
        shape = "box";
    } else if (dynamic_cast<const BlockStmtNode*>(node)) {
        color = "lightgray";
        label = "Block";
        shape = "box";
    } else if (dynamic_cast<const ExprStmtNode*>(node)) {
        color = "lightgray";
        label = "ExprStmt";
        shape = "box";
    } else if (dynamic_cast<const ParamNode*>(node)) {
        color = "lightsalmon";
        auto param = dynamic_cast<const ParamNode*>(node);
        label = "Param: " + param->getType() + " " + param->getName();
        shape = "box";
    } else {
        label = "Node";
        shape = "box";
    }
    
    out << "  " << nodeId << " [label=\"" << label << "\", shape=" << shape 
        << ", style=filled, fillcolor=" << color << "];\n";
    
    if (auto program = dynamic_cast<const ProgramNode*>(node)) {
        for (const auto& decl : program->getDeclarations()) {
            std::string childId = generateNodeId();
            visitNode(decl.get(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"decl\"];\n";
        }
    } else if (auto func = dynamic_cast<const FunctionDeclNode*>(node)) {
        for (const auto& param : func->getParameters()) {
            std::string childId = generateNodeId();
            visitNode(param.get(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"param\"];\n";
        }
        if (func->getBody()) {
            std::string childId = generateNodeId();
            visitNode(func->getBody(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"body\"];\n";
        }
    } else if (auto str = dynamic_cast<const StructDeclNode*>(node)) {
        for (const auto& field : str->getFields()) {
            std::string childId = generateNodeId();
            visitNode(field.get(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"field\"];\n";
        }
    } else if (auto bin = dynamic_cast<const BinaryExprNode*>(node)) {
        if (bin->getLeft()) {
            std::string childId = generateNodeId();
            visitNode(bin->getLeft(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"left\"];\n";
        }
        if (bin->getRight()) {
            std::string childId = generateNodeId();
            visitNode(bin->getRight(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"right\"];\n";
        }
    } else if (auto un = dynamic_cast<const UnaryExprNode*>(node)) {
        if (un->getOperand()) {
            std::string childId = generateNodeId();
            visitNode(un->getOperand(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"operand\"];\n";
        }
    } else if (auto call = dynamic_cast<const CallExprNode*>(node)) {
        if (call->getCallee()) {
            std::string childId = generateNodeId();
            visitNode(call->getCallee(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"callee\"];\n";
        }
        int argNum = 0;
        for (const auto& arg : call->getArguments()) {
            std::string childId = generateNodeId();
            visitNode(arg.get(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"arg" << argNum++ << "\"];\n";
        }
    } else if (auto assign = dynamic_cast<const AssignmentExprNode*>(node)) {
        if (assign->getTarget()) {
            std::string childId = generateNodeId();
            visitNode(assign->getTarget(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"target\"];\n";
        }
        if (assign->getValue()) {
            std::string childId = generateNodeId();
            visitNode(assign->getValue(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"value\"];\n";
        }
    } else if (auto block = dynamic_cast<const BlockStmtNode*>(node)) {
        int stmtNum = 0;
        for (const auto& stmt : block->getStatements()) {
            std::string childId = generateNodeId();
            visitNode(stmt.get(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"stmt" << stmtNum++ << "\"];\n";
        }
    } else if (auto ifStmt = dynamic_cast<const IfStmtNode*>(node)) {
        if (ifStmt->getCondition()) {
            std::string childId = generateNodeId();
            visitNode(ifStmt->getCondition(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"condition\"];\n";
        }
        if (ifStmt->getThenBranch()) {
            std::string childId = generateNodeId();
            visitNode(ifStmt->getThenBranch(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"then\"];\n";
        }
        if (ifStmt->getElseBranch()) {
            std::string childId = generateNodeId();
            visitNode(ifStmt->getElseBranch(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"else\"];\n";
        }
    } else if (auto whileStmt = dynamic_cast<const WhileStmtNode*>(node)) {
        if (whileStmt->getCondition()) {
            std::string childId = generateNodeId();
            visitNode(whileStmt->getCondition(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"condition\"];\n";
        }
        if (whileStmt->getBody()) {
            std::string childId = generateNodeId();
            visitNode(whileStmt->getBody(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"body\"];\n";
        }
    } else if (auto forStmt = dynamic_cast<const ForStmtNode*>(node)) {
        if (forStmt->getInit()) {
            std::string childId = generateNodeId();
            visitNode(forStmt->getInit(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"init\"];\n";
        }
        if (forStmt->getCondition()) {
            std::string childId = generateNodeId();
            visitNode(forStmt->getCondition(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"condition\"];\n";
        }
        if (forStmt->getUpdate()) {
            std::string childId = generateNodeId();
            visitNode(forStmt->getUpdate(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"update\"];\n";
        }
        if (forStmt->getBody()) {
            std::string childId = generateNodeId();
            visitNode(forStmt->getBody(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"body\"];\n";
        }
    } else if (auto returnStmt = dynamic_cast<const ReturnStmtNode*>(node)) {
        if (returnStmt->getValue()) {
            std::string childId = generateNodeId();
            visitNode(returnStmt->getValue(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"value\"];\n";
        }
    } else if (auto exprStmt = dynamic_cast<const ExprStmtNode*>(node)) {
        if (exprStmt->getExpression()) {
            std::string childId = generateNodeId();
            visitNode(exprStmt->getExpression(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"expr\"];\n";
        }
    } else if (auto varDecl = dynamic_cast<const VarDeclStmtNode*>(node)) {
        if (varDecl->hasInitializer()) {
            std::string childId = generateNodeId();
            visitNode(varDecl->getInitializer(), out);
            out << "  " << nodeId << " -> " << childId << " [label=\"init\"];\n";
        }
    }
}

std::string DOTPrinter::visualize(const ProgramNode* program) {
    std::ostringstream out;
    out << "digraph AST {\n";
    out << "  node [fontname=\"Courier\", fontsize=10];\n";
    out << "  edge [fontname=\"Courier\", fontsize=8];\n\n";
    
    nodeCounter = 0;
    if (program) {
        visitNode(program, out);
    }
    
    out << "}\n";
    return out.str();
}

// JSON
std::string JSONPrinter::escapeJSON(const std::string& str) {
    std::ostringstream out;
    for (char c : str) {
        switch (c) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << c;
        }
    }
    return out.str();
}

void JSONPrinter::visitNode(const ASTNode* node, std::ostringstream& out, int indent) {
    if (!node) {
        out << "null";
        return;
    }
    
    std::string indentStr(indent, ' ');
    std::string indentStr2(indent + 2, ' ');
    
    out << "{\n";
    
    if (dynamic_cast<const ProgramNode*>(node)) {
        auto prog = dynamic_cast<const ProgramNode*>(node);
        out << indentStr2 << "\"type\": \"Program\",\n";
        out << indentStr2 << "\"line\": " << prog->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << prog->getColumn() << ",\n";
        out << indentStr2 << "\"declarations\": [\n";
        for (size_t i = 0; i < prog->getDeclarations().size(); ++i) {
            visitNode(prog->getDeclarations()[i].get(), out, indent + 4);
            if (i < prog->getDeclarations().size() - 1) out << ",";
            out << "\n";
        }
        out << indentStr2 << "]\n";
    }
    else if (dynamic_cast<const FunctionDeclNode*>(node)) {
        auto func = dynamic_cast<const FunctionDeclNode*>(node);
        out << indentStr2 << "\"type\": \"FunctionDecl\",\n";
        out << indentStr2 << "\"line\": " << func->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << func->getColumn() << ",\n";
        out << indentStr2 << "\"name\": \"" << escapeJSON(func->getName()) << "\",\n";
        out << indentStr2 << "\"returnType\": \"" << escapeJSON(func->getReturnType()) << "\",\n";
        out << indentStr2 << "\"parameters\": [\n";
        for (size_t i = 0; i < func->getParameters().size(); ++i) {
            visitNode(func->getParameters()[i].get(), out, indent + 4);
            if (i < func->getParameters().size() - 1) out << ",";
            out << "\n";
        }
        out << indentStr2 << "],\n";
        out << indentStr2 << "\"body\": ";
        visitNode(func->getBody(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const ParamNode*>(node)) {
        auto param = dynamic_cast<const ParamNode*>(node);
        out << indentStr2 << "\"type\": \"Param\",\n";
        out << indentStr2 << "\"line\": " << param->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << param->getColumn() << ",\n";
        out << indentStr2 << "\"typeName\": \"" << escapeJSON(param->getType()) << "\",\n";
        out << indentStr2 << "\"name\": \"" << escapeJSON(param->getName()) << "\"\n";
    }
    else if (dynamic_cast<const BlockStmtNode*>(node)) {
        auto block = dynamic_cast<const BlockStmtNode*>(node);
        out << indentStr2 << "\"type\": \"Block\",\n";
        out << indentStr2 << "\"line\": " << block->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << block->getColumn() << ",\n";
        out << indentStr2 << "\"statements\": [\n";
        for (size_t i = 0; i < block->getStatements().size(); ++i) {
            visitNode(block->getStatements()[i].get(), out, indent + 4);
            if (i < block->getStatements().size() - 1) out << ",";
            out << "\n";
        }
        out << indentStr2 << "]\n";
    }
    else if (dynamic_cast<const VarDeclStmtNode*>(node)) {
        auto var = dynamic_cast<const VarDeclStmtNode*>(node);
        out << indentStr2 << "\"type\": \"VarDecl\",\n";
        out << indentStr2 << "\"line\": " << var->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << var->getColumn() << ",\n";
        out << indentStr2 << "\"typeName\": \"" << escapeJSON(var->getType()) << "\",\n";
        out << indentStr2 << "\"name\": \"" << escapeJSON(var->getName()) << "\",\n";
        out << indentStr2 << "\"initializer\": ";
        if (var->hasInitializer()) {
            visitNode(var->getInitializer(), out, indent + 2);
        } else {
            out << "null";
        }
        out << "\n";
    }
    else if (dynamic_cast<const ReturnStmtNode*>(node)) {
        auto ret = dynamic_cast<const ReturnStmtNode*>(node);
        out << indentStr2 << "\"type\": \"ReturnStmt\",\n";
        out << indentStr2 << "\"line\": " << ret->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << ret->getColumn() << ",\n";
        out << indentStr2 << "\"value\": ";
        if (ret->hasValue()) {
            visitNode(ret->getValue(), out, indent + 2);
        } else {
            out << "null";
        }
        out << "\n";
    }
    else if (dynamic_cast<const WhileStmtNode*>(node)) {
        auto whileStmt = dynamic_cast<const WhileStmtNode*>(node);
        out << indentStr2 << "\"type\": \"WhileStmt\",\n";
        out << indentStr2 << "\"line\": " << whileStmt->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << whileStmt->getColumn() << ",\n";
        out << indentStr2 << "\"condition\": ";
        visitNode(whileStmt->getCondition(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"body\": ";
        visitNode(whileStmt->getBody(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const ForStmtNode*>(node)) {
        auto forStmt = dynamic_cast<const ForStmtNode*>(node);
        out << indentStr2 << "\"type\": \"ForStmt\",\n";
        out << indentStr2 << "\"line\": " << forStmt->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << forStmt->getColumn() << ",\n";
        out << indentStr2 << "\"init\": ";
        if (forStmt->hasInit()) {
            visitNode(forStmt->getInit(), out, indent + 2);
        } else {
            out << "null";
        }
        out << ",\n";
        out << indentStr2 << "\"condition\": ";
        if (forStmt->hasCondition()) {
            visitNode(forStmt->getCondition(), out, indent + 2);
        } else {
            out << "null";
        }
        out << ",\n";
        out << indentStr2 << "\"update\": ";
        if (forStmt->hasUpdate()) {
            visitNode(forStmt->getUpdate(), out, indent + 2);
        } else {
            out << "null";
        }
        out << ",\n";
        out << indentStr2 << "\"body\": ";
        visitNode(forStmt->getBody(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const IfStmtNode*>(node)) {
        auto ifStmt = dynamic_cast<const IfStmtNode*>(node);
        out << indentStr2 << "\"type\": \"IfStmt\",\n";
        out << indentStr2 << "\"line\": " << ifStmt->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << ifStmt->getColumn() << ",\n";
        out << indentStr2 << "\"condition\": ";
        visitNode(ifStmt->getCondition(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"then\": ";
        visitNode(ifStmt->getThenBranch(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"else\": ";
        if (ifStmt->hasElse()) {
            visitNode(ifStmt->getElseBranch(), out, indent + 2);
        } else {
            out << "null";
        }
        out << "\n";
    }
    else if (dynamic_cast<const ExprStmtNode*>(node)) {
        auto exprStmt = dynamic_cast<const ExprStmtNode*>(node);
        out << indentStr2 << "\"type\": \"ExprStmt\",\n";
        out << indentStr2 << "\"line\": " << exprStmt->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << exprStmt->getColumn() << ",\n";
        out << indentStr2 << "\"expression\": ";
        visitNode(exprStmt->getExpression(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const BinaryExprNode*>(node)) {
        auto bin = dynamic_cast<const BinaryExprNode*>(node);
        out << indentStr2 << "\"type\": \"Binary\",\n";
        out << indentStr2 << "\"line\": " << bin->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << bin->getColumn() << ",\n";
        out << indentStr2 << "\"operator\": \"" << token_type_to_string(bin->getOperator()) << "\",\n";
        out << indentStr2 << "\"left\": ";
        visitNode(bin->getLeft(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"right\": ";
        visitNode(bin->getRight(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const UnaryExprNode*>(node)) {
        auto un = dynamic_cast<const UnaryExprNode*>(node);
        out << indentStr2 << "\"type\": \"Unary\",\n";
        out << indentStr2 << "\"line\": " << un->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << un->getColumn() << ",\n";
        out << indentStr2 << "\"operator\": \"" << token_type_to_string(un->getOperator()) << "\",\n";
        out << indentStr2 << "\"operand\": ";
        visitNode(un->getOperand(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const AssignmentExprNode*>(node)) {
        auto assign = dynamic_cast<const AssignmentExprNode*>(node);
        out << indentStr2 << "\"type\": \"Assignment\",\n";
        out << indentStr2 << "\"line\": " << assign->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << assign->getColumn() << ",\n";
        out << indentStr2 << "\"operator\": \"" << token_type_to_string(assign->getOperator()) << "\",\n";
        out << indentStr2 << "\"target\": ";
        visitNode(assign->getTarget(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"value\": ";
        visitNode(assign->getValue(), out, indent + 2);
        out << "\n";
    }
    else if (dynamic_cast<const CallExprNode*>(node)) {
        auto call = dynamic_cast<const CallExprNode*>(node);
        out << indentStr2 << "\"type\": \"Call\",\n";
        out << indentStr2 << "\"line\": " << call->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << call->getColumn() << ",\n";
        out << indentStr2 << "\"callee\": ";
        visitNode(call->getCallee(), out, indent + 2);
        out << ",\n";
        out << indentStr2 << "\"arguments\": [\n";
        for (size_t i = 0; i < call->getArguments().size(); ++i) {
            visitNode(call->getArguments()[i].get(), out, indent + 4);
            if (i < call->getArguments().size() - 1) out << ",";
            out << "\n";
        }
        out << indentStr2 << "]\n";
    }
    else if (dynamic_cast<const IdentifierExprNode*>(node)) {
        auto id = dynamic_cast<const IdentifierExprNode*>(node);
        out << indentStr2 << "\"type\": \"Identifier\",\n";
        out << indentStr2 << "\"line\": " << id->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << id->getColumn() << ",\n";
        out << indentStr2 << "\"name\": \"" << escapeJSON(id->getName()) << "\"\n";
    }
    else if (dynamic_cast<const LiteralExprNode*>(node)) {
        auto lit = dynamic_cast<const LiteralExprNode*>(node);
        out << indentStr2 << "\"type\": \"Literal\",\n";
        out << indentStr2 << "\"line\": " << lit->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << lit->getColumn() << ",\n";
        out << indentStr2 << "\"value\": ";
        
        if (auto intVal = lit->getValue<int>()) {
            out << *intVal;
        } else if (auto floatVal = lit->getValue<double>()) {
            out << std::fixed << std::setprecision(6) << *floatVal;
        } else if (auto strVal = lit->getValue<std::string>()) {
            out << "\"" << escapeJSON(*strVal) << "\"";
        } else if (auto boolVal = lit->getValue<bool>()) {
            out << (*boolVal ? "true" : "false");
        } else {
            out << "null";
        }
        out << "\n";
    }
    else {
        out << indentStr2 << "\"type\": \"Node\",\n";
        out << indentStr2 << "\"line\": " << node->getLine() << ",\n";
        out << indentStr2 << "\"column\": " << node->getColumn() << "\n";
    }
    
    out << indentStr << "}";
}

std::string JSONPrinter::visualize(const ProgramNode* program) {
    std::ostringstream out;
    out << "{\n";
    out << "  \"ast\": ";
    visitNode(program, out, 2);
    out << "\n}\n";
    return out.str();
}

// Генератор Кода
CodeGenerator::CodeGenerator() : indentLevel(0) {}

void CodeGenerator::indent() {
    indentLevel++;
}

void CodeGenerator::dedent() {
    if (indentLevel > 0) indentLevel--;
}

void CodeGenerator::generateProgram(const ProgramNode* program) {
    for (const auto& decl : program->getDeclarations()) {
        generateDeclaration(decl.get());
        out << "\n";
    }
}

void CodeGenerator::generateDeclaration(const DeclarationNode* decl) {
    if (auto func = dynamic_cast<const FunctionDeclNode*>(decl)) {
        generateFunctionDecl(func);
    } else if (auto structDecl = dynamic_cast<const StructDeclNode*>(decl)) {
        generateStructDecl(structDecl);
    }
}

void CodeGenerator::generateFunctionDecl(const FunctionDeclNode* func) {
    out << "fn " << func->getName() << "(";
    
    const auto& params = func->getParameters();
    for (size_t i = 0; i < params.size(); ++i) {
        generateParam(params[i].get());
        if (i < params.size() - 1) {
            out << ", ";
        }
    }
    out << ")";
    
    if (func->getReturnType() != "void") {
        out << " -> " << func->getReturnType();
    }
    out << " ";
    
    generateBlock(func->getBody());
}

void CodeGenerator::generateStructDecl(const StructDeclNode* structDecl) {
    out << "struct " << structDecl->getName() << " {\n";
    indent();
    
    for (const auto& field : structDecl->getFields()) {
        for (int i = 0; i < indentLevel; i++) out << "    ";
        generateVarDecl(field.get());
    }
    
    dedent();
    for (int i = 0; i < indentLevel; i++) out << "    ";
    out << "}\n";
}

void CodeGenerator::generateVarDecl(const VarDeclStmtNode* varDecl) {
    out << varDecl->getType() << " " << varDecl->getName();
    if (varDecl->hasInitializer()) {
        out << " = ";
        generateExpression(varDecl->getInitializer());
    }
    out << ";\n";
}

void CodeGenerator::generateStatement(const StatementNode* stmt) {
    if (auto block = dynamic_cast<const BlockStmtNode*>(stmt)) {
        generateBlock(block);
    } else if (auto ifStmt = dynamic_cast<const IfStmtNode*>(stmt)) {
        generateIfStmt(ifStmt);
    } else if (auto whileStmt = dynamic_cast<const WhileStmtNode*>(stmt)) {
        generateWhileStmt(whileStmt);
    } else if (auto forStmt = dynamic_cast<const ForStmtNode*>(stmt)) {
        generateForStmt(forStmt);
    } else if (auto returnStmt = dynamic_cast<const ReturnStmtNode*>(stmt)) {
        generateReturnStmt(returnStmt);
    } else if (auto exprStmt = dynamic_cast<const ExprStmtNode*>(stmt)) {
        generateExprStmt(exprStmt);
    } else if (auto varDecl = dynamic_cast<const VarDeclStmtNode*>(stmt)) {
        generateVarDecl(varDecl);
    }
}

void CodeGenerator::generateBlock(const BlockStmtNode* block) {
    out << "{\n";
    indent();
    
    for (const auto& stmt : block->getStatements()) {
        for (int i = 0; i < indentLevel; i++) out << "    ";
        generateStatement(stmt.get());
    }
    
    dedent();
    for (int i = 0; i < indentLevel; i++) out << "    ";
    out << "}\n";
}

void CodeGenerator::generateIfStmt(const IfStmtNode* ifStmt) {
    out << "if (";
    generateExpression(ifStmt->getCondition());
    out << ") ";
    
    if (dynamic_cast<const BlockStmtNode*>(ifStmt->getThenBranch())) {
        generateStatement(ifStmt->getThenBranch());
    } else {
        out << "\n";
        indent();
        for (int i = 0; i < indentLevel; i++) out << "    ";
        generateStatement(ifStmt->getThenBranch());
        dedent();
    }
    
    if (ifStmt->hasElse()) {
        out << " else ";
        if (dynamic_cast<const BlockStmtNode*>(ifStmt->getElseBranch())) {
            generateStatement(ifStmt->getElseBranch());
        } else {
            out << "\n";
            indent();
            for (int i = 0; i < indentLevel; i++) out << "    ";
            generateStatement(ifStmt->getElseBranch());
            dedent();
        }
    }
}

void CodeGenerator::generateWhileStmt(const WhileStmtNode* whileStmt) {
    out << "while (";
    generateExpression(whileStmt->getCondition());
    out << ") ";
    
    if (dynamic_cast<const BlockStmtNode*>(whileStmt->getBody())) {
        generateStatement(whileStmt->getBody());
    } else {
        out << "\n";
        indent();
        for (int i = 0; i < indentLevel; i++) out << "    ";
        generateStatement(whileStmt->getBody());
        dedent();
    }
}

void CodeGenerator::generateForStmt(const ForStmtNode* forStmt) {
    out << "for (";
    
    if (forStmt->hasInit()) {
        generateStatement(forStmt->getInit());
    } else {
        out << "; ";
    }
    
    if (forStmt->hasCondition()) {
        generateExpression(forStmt->getCondition());
        out << "; ";
    } else {
        out << "; ";
    }
    
    if (forStmt->hasUpdate()) {
        generateExpression(forStmt->getUpdate());
    }
    out << ") ";
    
    if (dynamic_cast<const BlockStmtNode*>(forStmt->getBody())) {
        generateStatement(forStmt->getBody());
    } else {
        out << "\n";
        indent();
        for (int i = 0; i < indentLevel; i++) out << "    ";
        generateStatement(forStmt->getBody());
        dedent();
    }
}

void CodeGenerator::generateReturnStmt(const ReturnStmtNode* returnStmt) {
    out << "return";
    if (returnStmt->hasValue()) {
        out << " ";
        generateExpression(returnStmt->getValue());
    }
    out << ";\n";
}

void CodeGenerator::generateExprStmt(const ExprStmtNode* exprStmt) {
    generateExpression(exprStmt->getExpression());
    out << ";\n";
}

void CodeGenerator::generateExpression(const ExpressionNode* expr) {
    if (auto bin = dynamic_cast<const BinaryExprNode*>(expr)) {
        generateBinaryExpr(bin);
    } else if (auto un = dynamic_cast<const UnaryExprNode*>(expr)) {
        generateUnaryExpr(un);
    } else if (auto lit = dynamic_cast<const LiteralExprNode*>(expr)) {
        generateLiteralExpr(lit);
    } else if (auto id = dynamic_cast<const IdentifierExprNode*>(expr)) {
        generateIdentifierExpr(id);
    } else if (auto call = dynamic_cast<const CallExprNode*>(expr)) {
        generateCallExpr(call);
    } else if (auto assign = dynamic_cast<const AssignmentExprNode*>(expr)) {
        generateAssignmentExpr(assign);
    }
}

void CodeGenerator::generateBinaryExpr(const BinaryExprNode* bin) {
    out << "(";
    generateExpression(bin->getLeft());
    out << " " << token_type_to_string(bin->getOperator()) << " ";
    generateExpression(bin->getRight());
    out << ")";
}

void CodeGenerator::generateUnaryExpr(const UnaryExprNode* un) {
    out << token_type_to_string(un->getOperator());
    generateExpression(un->getOperand());
}

void CodeGenerator::generateLiteralExpr(const LiteralExprNode* lit) {
    if (auto intVal = lit->getValue<int>()) {
        out << *intVal;
    } else if (auto floatVal = lit->getValue<double>()) {
        out << std::fixed << std::setprecision(6) << *floatVal;
    } else if (auto strVal = lit->getValue<std::string>()) {
        out << "\"" << *strVal << "\"";
    } else if (auto boolVal = lit->getValue<bool>()) {
        out << (*boolVal ? "true" : "false");
    }
}

void CodeGenerator::generateIdentifierExpr(const IdentifierExprNode* id) {
    out << id->getName();
}

void CodeGenerator::generateCallExpr(const CallExprNode* call) {
    generateExpression(call->getCallee());
    out << "(";
    const auto& args = call->getArguments();
    for (size_t i = 0; i < args.size(); ++i) {
        generateExpression(args[i].get());
        if (i < args.size() - 1) {
            out << ", ";
        }
    }
    out << ")";
}

void CodeGenerator::generateAssignmentExpr(const AssignmentExprNode* assign) {
    generateExpression(assign->getTarget());
    out << " " << token_type_to_string(assign->getOperator()) << " ";
    generateExpression(assign->getValue());
}

void CodeGenerator::generateParam(const ParamNode* param) {
    out << param->getType() << " " << param->getName();
}

std::string CodeGenerator::generate(const ProgramNode* program) {
    out.str("");
    out.clear();
    generateProgram(program);
    return out.str();
}