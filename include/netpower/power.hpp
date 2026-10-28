#pragma once

#include "netpower/circuit.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace netpower {

class PowerModelError : public std::runtime_error {
public:
    explicit PowerModelError(const std::string& message) : std::runtime_error(message) {}
};

struct PowerConfig {
    double voltage_V{1.0};
    double frequency_Hz{1e8};
    double default_capacitance_fF{2.0};
    std::string capacitance_file;
};

struct PowerSummary {
    double total_power_nW{0.0};
    double total_power_W{0.0};
    double voltage_V{0.0};
    double frequency_Hz{0.0};
    double default_capacitance_fF{0.0};
    size_t overridden_capacitance_count{0};
};

std::unordered_map<std::string, double> load_capacitance_overrides(
    const std::string& filepath, const Circuit& circuit);

PowerSummary calculate_power(Circuit& circuit, const PowerConfig& config);

}  // namespace netpower

