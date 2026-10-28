#pragma once

#include "netpower/power.hpp"
#include "netpower/simulator.hpp"

#include <optional>
#include <string>

namespace netpower {

struct CliOptions {
    std::string netlist_file;
    uint64_t vectors{0};
    uint64_t seed{0};
    double voltage_V{0.0};
    double frequency_Hz{0.0};
    double default_capacitance_fF{0.0};
    std::string output_dir;
    std::string capacitance_file;
    bool help_requested{false};
};

class CliParser {
public:
    static CliOptions parse(int argc, char* argv[]);
    static void print_help(std::string_view program_name);
};

}  // namespace netpower

