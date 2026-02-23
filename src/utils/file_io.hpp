#ifndef FILE_IO_H
#define FILE_IO_H

#include <string>
#include <vector>
#include <stdexcept> 
#include "../lexer/tokens.hpp"

class FileIO {
public:
    static bool exists(const std::string& path);
    static std::string read(const std::string& path);
    static void write(const std::string& content, const std::string& path);
    static void write_tokens(const std::vector<Token>& tokens, const std::string& path);
    
    class Error : public std::runtime_error {
    public:
        explicit Error(const std::string& message) : std::runtime_error(message) {}
    };
};

#endif