#include <iostream>
#include <string>
#include <vector>
#include "lexer/lexer.hpp"
#include "preprocessor/preprocessor.hpp"
#include "utils/cli.hpp"
#include "utils/file_io.hpp"

int run_preprocess(const std::string& input_file, const std::string& output_file) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }
        
        std::string source = FileIO::read(input_file);
        Preprocessor preprocessor(source);
        std::string processed = preprocessor.process();
        
        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }
        
        FileIO::write(processed, output_file);
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int run_lexer(const std::string& input_file, const std::string& output_file) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }
        
        std::string source = FileIO::read(input_file);
        
        // Запускаем препроцессор перед лексером
        Preprocessor preprocessor(source);
        std::string processed_source = preprocessor.process();
        
        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }
        
        // Запускаем лексер на обработанном исходнике
        Lexer lexer(processed_source);
        std::vector<Token> tokens;
        
        while (true) {
            Token token = lexer.lexer_next_token();
            tokens.push_back(token);
            
            if (token.type == TokenType::TOKEN_END_OF_FILE) {
                break;
            }
            
            if (token.type == TokenType::TOKEN_ERROR) {
                std::cerr << "Ошибка: " << token.lexeme << "\n";
            }
        }
        
        FileIO::write_tokens(tokens, output_file);
        std::cout << "Успешно обработано " << tokens.size() - 1 
                  << " токенов. Результат в: " << output_file << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int main(int argc, char* argv[]) {
    CommandLineOptions options = CLI::parse(argc, argv);
    
    if (options.help_requested) {
        CLI::print_usage(argv[0]);
        return 0;
    }
    
    if (options.command == "version") {
        CLI::print_version();
        return 0;
    }
    
    if (options.command == "preprocess") {
        if (options.input_file.empty() || options.output_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_preprocess(options.input_file, options.output_file);
    } 
    else if (options.command == "lex") {
        if (options.input_file.empty() || options.output_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_lexer(options.input_file, options.output_file);
    } 
    else {
        std::cerr << "Неизвестная команда: " << options.command << "\n";
        CLI::print_usage(argv[0]);
        return 1;
    }
}