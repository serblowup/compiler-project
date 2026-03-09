#include "cli.hpp"
#include <iostream>
#include <cstring>
#include <getopt.h>
#include <cstdlib>

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
        } else if (arg == "--format" && i + 1 < argc) {
            options.ast_format = argv[++i];
        } else if (arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--max-errors" && i + 1 < argc) {
            options.max_errors = std::atoi(argv[++i]);
            if (options.max_errors <= 0) {
                options.max_errors = 100;
            }
        }
    }
    
    return options;
}

void CLI::print_usage(const char* program_name) {
    std::cerr << "Использование: " << program_name << " <команда> [опции]\n";
    std::cerr << "Команды:\n";
    std::cerr << "  lex --input <файл> --output <файл>           - запустить лексер (с предварительной обработкой)\n";
    std::cerr << "  preprocess --input <файл> --output <файл>    - только препроцессор\n";
    std::cerr << "  parse --input <файл> --output <файл> [опции] - запустить парсер\n";
    std::cerr << "  help                                          - показать справку\n";
    std::cerr << "\nОпции для parse:\n";
    std::cerr << "  --format text|dot|json        - формат вывода AST (по умолчанию: text)\n";
    std::cerr << "  --verbose                      - подробный вывод\n";
    std::cerr << "  --max-errors <число>           - максимальное количество ошибок (по умолчанию: 100)\n";
    std::cerr << "\nОбщие опции:\n";
    std::cerr << "  --input <файл>     - входной файл\n";
    std::cerr << "  --output <файл>    - выходной файл\n";
    std::cerr << "\nПримеры:\n";
    std::cerr << "  " << program_name << " lex --input examples/hello.src --output tokens.txt\n";
    std::cerr << "  " << program_name << " preprocess --input examples/hello.src --output hello.i\n";
    std::cerr << "  " << program_name << " parse --input examples/factorial.src --output ast.txt\n";
    std::cerr << "  " << program_name << " parse --input examples/factorial.src --format dot --output ast.dot --verbose\n";
    std::cerr << "  " << program_name << " parse --input examples/error.src --max-errors 50 --output ast.txt\n";
}

void CLI::print_version() {
    std::cout << "MiniCompiler version 2.0.0 (Sprint 2)\n";
}
