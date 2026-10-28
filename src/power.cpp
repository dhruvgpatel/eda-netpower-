#include "netpower/power.hpp"

#include <cmath>
#include <fstream>
#include <sstream>

namespace netpower {

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::unordered_map<std::string, double> load_capacitance_overrides(
    const std::string& filepath, const Circuit& circuit) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw PowerModelError("cannot open capacitance override file: '" + filepath + "'");
    }

    std::unordered_map<std::string, double> overrides;
    std::string line;
    size_t line_num = 0;

    // Read header
    if (!std::getline(file, line)) {
        throw PowerModelError("empty capacitance override file: '" + filepath + "'");
    }
    line_num++;
    line = trim(line);
    if (line != "net,capacitance_fF") {
        throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                              ": invalid header; expected 'net,capacitance_fF', got '" + line + "'");
    }

    while (std::getline(file, line)) {
        line_num++;
        line = trim(line);
        if (line.empty()) continue;

        size_t comma_pos = line.find(',');
        if (comma_pos == std::string::npos) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                                  ": malformed line; expected 'net,capacitance_fF'");
        }

        std::string net_name = trim(line.substr(0, comma_pos));
        std::string cap_str = trim(line.substr(comma_pos + 1));

        if (net_name.empty()) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) + ": missing net name");
        }

        if (!circuit.find_net_index(net_name).has_value()) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                                  ": unknown net '" + net_name + "'");
        }

        if (overrides.find(net_name) != overrides.end()) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                                  ": duplicate capacitance entry for net '" + net_name + "'");
        }

        double cap_val = 0.0;
        try {
            size_t idx = 0;
            cap_val = std::stod(cap_str, &idx);
            if (idx != cap_str.size()) {
                throw std::invalid_argument("trailing characters");
            }
        } catch (const std::exception&) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                                  ": invalid capacitance value '" + cap_str + "'");
        }

        if (std::isnan(cap_val) || std::isinf(cap_val) || cap_val < 0.0) {
            throw PowerModelError(filepath + ":" + std::to_string(line_num) +
                                  ": capacitance must be non-negative and finite, got " + cap_str);
        }

        overrides[net_name] = cap_val;
    }

    return overrides;
}

PowerSummary calculate_power(Circuit& circuit, const PowerConfig& config) {
    if (std::isnan(config.voltage_V) || std::isinf(config.voltage_V) || config.voltage_V <= 0.0) {
        throw PowerModelError("voltage must be strictly positive and finite, got " + std::to_string(config.voltage_V));
    }
    if (std::isnan(config.frequency_Hz) || std::isinf(config.frequency_Hz) || config.frequency_Hz <= 0.0) {
        throw PowerModelError("frequency must be strictly positive and finite, got " + std::to_string(config.frequency_Hz));
    }
    if (std::isnan(config.default_capacitance_fF) || std::isinf(config.default_capacitance_fF) || config.default_capacitance_fF < 0.0) {
        throw PowerModelError("default capacitance must be non-negative and finite, got " + std::to_string(config.default_capacitance_fF));
    }

    std::unordered_map<std::string, double> overrides;
    if (!config.capacitance_file.empty()) {
        overrides = load_capacitance_overrides(config.capacitance_file, circuit);
    }

    PowerSummary summary;
    summary.voltage_V = config.voltage_V;
    summary.frequency_Hz = config.frequency_Hz;
    summary.default_capacitance_fF = config.default_capacitance_fF;
    summary.overridden_capacitance_count = overrides.size();

    const double v_squared = config.voltage_V * config.voltage_V;
    double total_p_w = 0.0;
    double total_p_nw = 0.0;

    for (auto& net : circuit.nets) {
        double c_fF = config.default_capacitance_fF;
        auto it = overrides.find(net.name);
        if (it != overrides.end()) {
            c_fF = it->second;
        }
        net.capacitance_fF = c_fF;

        // C_F = C_fF * 1e-15
        const double c_F = c_fF * 1e-15;
        // P_dynamic = 0.5 * alpha * C * V^2 * f
        const double p_W = 0.5 * net.alpha * c_F * v_squared * config.frequency_Hz;
        const double p_nW = p_W * 1e9;

        net.power_nW = p_nW;
        total_p_w += p_W;
        total_p_nw += p_nW;
    }

    summary.total_power_W = total_p_w;
    summary.total_power_nW = total_p_nw;
    return summary;
}

}  // namespace netpower

