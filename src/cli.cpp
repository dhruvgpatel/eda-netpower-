#include "netpower/cli.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace netpower {

void CliParser::print_help(std::string_view program_name) {
    std::cout << "Usage: " << program_name << " [options]\n\n"
              << "A Gate-Level Netlist Parser and Dynamic Power Estimator (C++17).\n\n"
              << "Required Options:\n"
              << "  --netlist <file>                 Path to structural Verilog netlist file\n"
              << "  --vectors <count>                Number of random simulation vectors (>= 2)\n"
              << "  --seed <int>                     PRNG seed for deterministic simulation\n"
              << "  --voltage <volts>                Operating voltage in Volts (> 0)\n"
              << "  --frequency-hz <hertz>           Operating frequency in Hertz (> 0)\n"
              << "  --default-capacitance-ff <val>   Default net capacitance in femtofarads (>= 0)\n"
              << "  --output-dir <dir>               Directory where summary.txt and net_activity.csv are written\n\n"
              << "Optional Options:\n"
              << "  --capacitance-file <file>        CSV file specifying per-net capacitance overrides (net,capacitance_fF)\n"
              << "  -h, --help                       Show this help message and exit\n";
}

static uint64_t parse_uint64(std::string_view opt_name, const std::string& val_str) {
    try {
        size_t idx = 0;
        unsigned long long val = std::stoull(val_str, &idx);
        if (idx != val_str.size()) {
            throw std::invalid_argument("trailing characters");
        }
        return static_cast<uint64_t>(val);
    } catch (const std::exception&) {
        throw std::runtime_error("invalid integer value '" + val_str + "' for option '" + std::string(opt_name) + "'");
    }
}

static double parse_double(std::string_view opt_name, const std::string& val_str) {
    try {
        size_t idx = 0;
        double val = std::stod(val_str, &idx);
        if (idx != val_str.size()) {
            throw std::invalid_argument("trailing characters");
        }
        if (std::isnan(val) || std::isinf(val)) {
            throw std::invalid_argument("non-finite value");
        }
        return val;
    } catch (const std::exception&) {
        throw std::runtime_error("invalid numeric value '" + val_str + "' for option '" + std::string(opt_name) + "'");
    }
}

CliOptions CliParser::parse(int argc, char* argv[]) {
    CliOptions opts;
    bool has_netlist = false;
    bool has_vectors = false;
    bool has_seed = false;
    bool has_voltage = false;
    bool has_frequency = false;
    bool has_default_cap = false;
    bool has_output_dir = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            opts.help_requested = true;
            return opts;
        }

        auto require_value = [&](std::string_view opt_name) -> std::string {
            if (i + 1 >= argc) {
                throw std::runtime_error("missing argument for option '" + std::string(opt_name) + "'");
            }
            return argv[++i];
        };

        if (arg == "--netlist") {
            opts.netlist_file = require_value("--netlist");
            has_netlist = true;
        } else if (arg == "--vectors") {
            std::string val = require_value("--vectors");
            opts.vectors = parse_uint64("--vectors", val);
            if (opts.vectors < 2) {
                throw std::runtime_error("option '--vectors' must be >= 2, got " + val);
            }
            has_vectors = true;
        } else if (arg == "--seed") {
            opts.seed = parse_uint64("--seed", require_value("--seed"));
            has_seed = true;
        } else if (arg == "--voltage") {
            std::string val = require_value("--voltage");
            opts.voltage_V = parse_double("--voltage", val);
            if (opts.voltage_V <= 0.0) {
                throw std::runtime_error("option '--voltage' must be strictly positive, got " + val);
            }
            has_voltage = true;
        } else if (arg == "--frequency-hz") {
            std::string val = require_value("--frequency-hz");
            opts.frequency_Hz = parse_double("--frequency-hz", val);
            if (opts.frequency_Hz <= 0.0) {
                throw std::runtime_error("option '--frequency-hz' must be strictly positive, got " + val);
            }
            has_frequency = true;
        } else if (arg == "--default-capacitance-ff") {
            std::string val = require_value("--default-capacitance-ff");
            opts.default_capacitance_fF = parse_double("--default-capacitance-ff", val);
            if (opts.default_capacitance_fF < 0.0) {
                throw std::runtime_error("option '--default-capacitance-ff' must be non-negative, got " + val);
            }
            has_default_cap = true;
        } else if (arg == "--output-dir") {
            opts.output_dir = require_value("--output-dir");
            has_output_dir = true;
        } else if (arg == "--capacitance-file") {
            opts.capacitance_file = require_value("--capacitance-file");
        } else {
            throw std::runtime_error("unknown command line option '" + arg + "'");
        }
    }

    if (!has_netlist) throw std::runtime_error("missing required option '--netlist'");
    if (!has_vectors) throw std::runtime_error("missing required option '--vectors'");
    if (!has_seed) throw std::runtime_error("missing required option '--seed'");
    if (!has_voltage) throw std::runtime_error("missing required option '--voltage'");
    if (!has_frequency) throw std::runtime_error("missing required option '--frequency-hz'");
    if (!has_default_cap) throw std::runtime_error("missing required option '--default-capacitance-ff'");
    if (!has_output_dir) throw std::runtime_error("missing required option '--output-dir'");

    return opts;
}

}  // namespace netpower

