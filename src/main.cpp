#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/ast_visualizer.hpp"
#include "preprocessor/preprocessor.hpp"
#include "utils/cli.hpp"
#include "utils/file_io.hpp"

int run_preprocess(const std::string& input_file, const std::string& output_file) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "[Preprocessor] Ошибка: Входной файл '" << input_file << "' не найден\n";
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
        std::cout << "Препроцессор завершен. Результат в: " << output_file << "\n";
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[Preprocessor] Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int run_lexer(const std::string& input_file, const std::string& output_file) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "[Lexer] Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }
        
        std::string source = FileIO::read(input_file);
        
        Preprocessor preprocessor(source);
        std::string processed_source = preprocessor.process();
        
        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }
        
        Lexer lexer(processed_source);
        std::vector<Token> tokens;
        
        while (true) {
            Token token = lexer.lexer_next_token();
            tokens.push_back(token);
            
            if (token.type == TokenType::TOKEN_END_OF_FILE) {
                break;
            }
            
            if (token.type == TokenType::TOKEN_ERROR) {
                std::cerr << "[Lexer] Ошибка: " << token.lexeme << "\n";
            }
        }
        
        FileIO::write_tokens(tokens, output_file);
        std::cout << "Успешно обработано " << tokens.size() - 1 
                  << " токенов. Результат в: " << output_file << "\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[Lexer] Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int run_parser(const std::string& input_file, const std::string& output_file, 
               const std::string& format, bool verbose, int max_errors) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "[Parser] Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }
        
        std::string source = FileIO::read(input_file);
        
        if (verbose) {
            std::cout << "[Parser] Чтение файла: " << input_file << "\n";
        }
        
        Preprocessor preprocessor(source);
        std::string processed_source = preprocessor.process();
        
        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }
        
        if (verbose) {
            std::cout << "[Parser] Препроцессор завершен\n";
        }
        
        Lexer lexer(processed_source);
        std::vector<Token> tokens;
        
        while (true) {
            Token token = lexer.lexer_next_token();
            tokens.push_back(token);
            
            if (token.type == TokenType::TOKEN_END_OF_FILE) {
                break;
            }
            
            if (token.type == TokenType::TOKEN_ERROR) {
                std::cerr << "[Lexer] Ошибка: " << token.lexeme << "\n";
                return 1;
            }
        }
        
        if (verbose) {
            std::cout << "[Parser] Лексер завершен. Получено " << tokens.size() - 1 << " токенов\n";
        }
        
        Parser parser(tokens);
        parser.setMaxErrorCount(max_errors);
        std::unique_ptr<ProgramNode> ast = parser.parse();
        
        if (parser.hasErrors()) {
            std::cerr << "[Parser] Обнаружены синтаксические ошибки:\n";
            for (const auto& error : parser.getErrors()) {
                std::cerr << error.toString() << "\n";
            }
            
            if (verbose) {
                std::cerr << "\n" << parser.getMetrics().toString() << "\n";
            }
            
            return 1;
        }
        
        if (verbose) {
            std::cout << "[Parser] Парсер завершен. AST построен\n";
            std::cout << parser.getMetrics().toString() << "\n";
        }
        
        std::unique_ptr<ASTVisualizer> visualizer;
        
        if (format == "dot") {
            visualizer = std::make_unique<DOTPrinter>();
        } else if (format == "json") {
            visualizer = std::make_unique<JSONPrinter>();
        } else {
            visualizer = std::make_unique<PrettyPrinter>();
        }
        
        std::string ast_output = visualizer->visualize(ast.get());
        FileIO::write(ast_output, output_file);
        
        std::cout << "Парсер завершен. AST сохранен в: " << output_file << "\n";
        if (verbose) {
            std::cout << "Формат вывода: " << format << "\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "[Parser] Ошибка: " << e.what() << "\n";
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
    else if (options.command == "parse") {
        if (options.input_file.empty() || options.output_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_parser(options.input_file, options.output_file,
                          options.ast_format, options.verbose, options.max_errors);
    }
    else {
        std::cerr << "Неизвестная команда: " << options.command << "\n";
        CLI::print_usage(argv[0]);
        return 1;
    }
}
