#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include <utility>

struct CommandLineOptions {
    std::string command;
    std::string input_file;
    std::string output_file;
    std::string ast_format;
    std::vector<std::string> include_paths;
    std::vector<std::pair<std::string, std::string>> defines;
    bool verbose;
    bool help_requested;
    int max_errors;
    
    bool dump_symbols;
    bool show_types;
    std::string symbol_format;
    
    bool ir_optimize;
    bool ir_stats;
    std::string ir_format;
    
    CommandLineOptions() 
        : command(""), 
          input_file(""), 
          output_file(""), 
          ast_format("text"), 
          include_paths(), 
          defines(), 
          verbose(false), 
          help_requested(false),
          max_errors(100),
          dump_symbols(false),
          show_types(false),
          symbol_format("text"),
          ir_optimize(false),
          ir_stats(false),
          ir_format("text") {}
};

class CLI {
public:
    static CommandLineOptions parse(int argc, char* argv[]);
    static void print_usage(const char* program_name);
    static void print_version();
};

#endif
