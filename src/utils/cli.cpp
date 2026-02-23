#include "cli.hpp"
#include <iostream>
#include <cstring>
#include <getopt.h>

CommandLineOptions CLI::parse(int argc, char* argv[]) {
    CommandLineOptions options;
    
    if (argc < 2) {
        options.help_requested = true;
        return options;
    }
    
    options.command = argv[1];
    
    if (options.command == "help") {
        options.help_requested = true;
        return options;
    }
    
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--input" && i + 1 < argc) {
            options.input_file = argv[++i];
        } else if (arg == "--output" && i + 1 < argc) {
            options.output_file = argv[++i];
        }
    }
    
    return options;
}

void CLI::print_usage(const char* program_name) {
    std::cerr << "Использование: " << program_name << " <команда> [опции]\n";
    std::cerr << "Команды:\n";
    std::cerr << "  lex --input <файл> --output <файл>  - запустить лексер (с предварительной обработкой)\n";
    std::cerr << "  preprocess --input <файл> --output <файл> - только препроцессор\n";
    std::cerr << "  help                                          - показать справку\n";
    std::cerr << "\nОпции:\n";
    std::cerr << "  --input <файл>     - входной файл\n";
    std::cerr << "  --output <файл>    - выходной файл\n";
    std::cerr << "\nПримеры:\n";
    std::cerr << "  " << program_name << " lex --input examples/hello.src --output tokens.txt\n";
    std::cerr << "  " << program_name << " preprocess --input examples/hello.src --output hello.i\n";
}

void CLI::print_version() {
    std::cout << "MiniCompiler version 1.0.0 (Sprint 1)\n";
}