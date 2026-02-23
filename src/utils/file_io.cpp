#include "file_io.hpp"
#include <fstream>
#include <iostream>

bool FileIO::exists(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

std::string FileIO::read(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw Error("Не удалось открыть файл: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
}

void FileIO::write(const std::string& content, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw Error("Не удалось создать выходной файл: " + path);
    }
    out << content;
}

void FileIO::write_tokens(const std::vector<Token>& tokens, const std::string& path) {
    std::ofstream out(path);
    if (!out.is_open()) {
        throw Error("Не удалось создать выходной файл: " + path);
    }
    
    for (const auto& token : tokens) {
        out << token.toString() << "\n";
    }
}