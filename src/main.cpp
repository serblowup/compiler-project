#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include "parser/ast_visualizer.hpp"
#include "preprocessor/preprocessor.hpp"
#include "semantic/semantic_analyzer.hpp"
#include "ir/ir_generator.hpp"
#include "ir/ir_printer.hpp"
#include "ir/ir_optimizer.hpp"
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

        Parser parser(tokens, input_file, processed_source);
        parser.setMaxErrorCount(max_errors);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        std::unique_ptr<ASTVisualizer> visualizer;

        if (format == "dot") {
            visualizer = std::make_unique<DOTPrinter>();
        } else if (format == "json") {
            visualizer = std::make_unique<JSONPrinter>();
        } else {
            visualizer = std::make_unique<PrettyPrinter>();
        }

        std::string ast_output = visualizer->visualize(ast.get());
        
        if (output_file.empty() || output_file == "/dev/stdout") {
            std::cout << ast_output;
        } else {
            FileIO::write(ast_output, output_file);
        }

        if (verbose) {
            std::cerr << "Парсер завершен. AST сохранен" << std::endl;
        }

        return parser.hasErrors() ? 1 : 0;

    } catch (const std::exception& e) {
        std::cerr << "[Parser] Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int run_check(const std::string& input_file, const std::string& output_file,
              const std::string& format, bool verbose, int max_errors,
              bool dump_symbols, bool show_types, const std::string& symbol_format) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "[Semantic] Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }

        std::string source = FileIO::read(input_file);

        if (verbose) {
            std::cout << "[Semantic] Чтение файла: " << input_file << "\n";
        }

        Preprocessor preprocessor(source);
        std::string processed_source = preprocessor.process();

        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }

        if (verbose) {
            std::cout << "[Semantic] Препроцессор завершен\n";
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
            std::cout << "[Semantic] Лексер завершен. Получено " << tokens.size() - 1 << " токенов\n";
        }

        Parser parser(tokens, input_file, processed_source);
        parser.setMaxErrorCount(max_errors);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        semantic::SemanticAnalyzer analyzer(input_file, processed_source);
        analyzer.analyze(ast.get());

        if (analyzer.hasErrors()) {
            std::cerr << analyzer.getAllErrors();
        }

        if (verbose) {
            std::cout << "[Semantic] Семантический анализ завершен\n";
        }

        std::ostringstream output;

        if (format == "dot" && !dump_symbols && !show_types) {
            std::unique_ptr<ASTVisualizer> visualizer = std::make_unique<DOTPrinter>();
            output << visualizer->visualize(ast.get());
        } else {
            if (show_types || (!dump_symbols && !verbose)) {
                if (format == "dot") {
                    std::unique_ptr<ASTVisualizer> visualizer = std::make_unique<DOTPrinter>();
                    output << visualizer->visualize(ast.get());
                } else if (format == "json") {
                    std::unique_ptr<ASTVisualizer> visualizer = std::make_unique<JSONPrinter>();
                    output << visualizer->visualize(ast.get());
                } else {
                    if (show_types) {
                        output << "[Разукрашенное AST]\n";
                    }
                    std::unique_ptr<ASTVisualizer> visualizer = std::make_unique<PrettyPrinter>();
                    output << visualizer->visualize(ast.get());
                }
                output << "\n";
            }

            if (dump_symbols) {
                auto snapshot = analyzer.getSymbolTableSnapshot();
                if (symbol_format == "json") {
                    output << snapshot.toJSON();
                } else {
                    output << "[Таблица символов]\n";
                    output << snapshot.toString();
                }
                output << "\n";
            }

            if (verbose) {
                output << analyzer.getValidationReport();
            }
        }

        if (output_file.empty() || output_file == "/dev/stdout") {
            std::cout << output.str();
        } else {
            FileIO::write(output.str(), output_file);
            if (verbose) {
                std::cout << "[Semantic] Результат сохранен в: " << output_file << "\n";
            }
        }

        return (parser.hasErrors() || analyzer.hasErrors()) ? 1 : 0;

    } catch (const std::exception& e) {
        std::cerr << "[Semantic] Ошибка: " << e.what() << "\n";
        return 1;
    }
}

int run_ir_generator(const std::string& input_file, const std::string& output_file,
                     const std::string& ir_format, bool optimize, bool stats, bool verbose, int max_errors) {
    try {
        if (!FileIO::exists(input_file)) {
            std::cerr << "[IR] Ошибка: Входной файл '" << input_file << "' не найден\n";
            return 1;
        }

        std::string source = FileIO::read(input_file);

        if (verbose) {
            std::cout << "[IR] Чтение файла: " << input_file << "\n";
        }

        Preprocessor preprocessor(source);
        std::string processed_source = preprocessor.process();

        if (preprocessor.has_errors()) {
            std::cerr << preprocessor.get_errors();
            return 1;
        }

        if (verbose) {
            std::cout << "[IR] Препроцессор завершен\n";
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
            std::cout << "[IR] Лексер завершен. Получено " << tokens.size() - 1 << " токенов\n";
        }

        Parser parser(tokens, input_file, processed_source);
        parser.setMaxErrorCount(max_errors);
        std::unique_ptr<ProgramNode> ast = parser.parse();

        if (parser.hasErrors()) {
            return 1;
        }

        semantic::SemanticAnalyzer analyzer(input_file, processed_source);
        analyzer.analyze(ast.get());

        if (analyzer.hasErrors()) {
            std::cerr << analyzer.getAllErrors();
            return 1;
        }

        if (verbose) {
            std::cout << "[IR] Семантический анализ завершен\n";
        }

        ir::IRGenerator generator(analyzer.getSymbolTable());
        ir::IRProgram* program = generator.generate(ast.get());

        if (optimize) {
            if (verbose) {
                std::cout << "[IR] Выполнение оптимизаций...\n";
            }
            ir::IROptimizer optimizer;
            optimizer.optimize(program);
            if (verbose) {
                std::cout << optimizer.getReport().toString();
            }
        }

        std::string output;
        
        if (ir_format == "dot") {
            output = ir::IRPrinter::toDot(program);
        } else if (ir_format == "json") {
            output = ir::IRPrinter::toJSON(program);
        } else {
            output = ir::IRPrinter::toString(program);
        }

        if (stats) {
            output += "\n" + ir::IRPrinter::getStats(program);
        }

        if (output_file.empty() || output_file == "/dev/stdout") {
            std::cout << output;
        } else {
            FileIO::write(output, output_file);
            if (verbose) {
                std::cout << "[IR] Результат сохранен в: " << output_file << "\n";
            }
        }

        delete program;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "[IR] Ошибка: " << e.what() << "\n";
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
        if (options.input_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_parser(options.input_file, options.output_file,
                         options.ast_format, options.verbose, options.max_errors);
    }
    else if (options.command == "check") {
        if (options.input_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_check(options.input_file, options.output_file,
                        options.ast_format, options.verbose, options.max_errors,
                        options.dump_symbols, options.show_types, options.symbol_format);
    }
    else if (options.command == "ir") {
        if (options.input_file.empty()) {
            CLI::print_usage(argv[0]);
            return 1;
        }
        return run_ir_generator(options.input_file, options.output_file,
                                options.ir_format, options.ir_optimize, options.ir_stats,
                                options.verbose, options.max_errors);
    }
    else {
        std::cerr << "Неизвестная команда: " << options.command << "\n";
        CLI::print_usage(argv[0]);
        return 1;
    }
}