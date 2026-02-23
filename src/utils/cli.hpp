#ifndef CLI_H
#define CLI_H

#include <string>
#include <vector>
#include <utility>

struct CommandLineOptions {
    std::string command;
    std::string input_file;
    std::string output_file;
    std::vector<std::string> include_paths;
    std::vector<std::pair<std::string, std::string>> defines;
    bool help_requested;
    
    CommandLineOptions() : help_requested(false) {}
};

class CLI {
public:
    static CommandLineOptions parse(int argc, char* argv[]);
    static void print_usage(const char* program_name);
    static void print_version();
};

#endif