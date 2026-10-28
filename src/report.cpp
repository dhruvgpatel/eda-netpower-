#include "netpower/report.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace netpower {

namespace fs = std::filesystem;

static std::string build_summary_text(const Circuit& circuit,
                                      const SimulationResult& sim_result,
                                      const PowerSummary& power_summary) {
    std::ostringstream oss;
    oss << "================================================================================\n"
        << "Netpower Dynamic Power Estimation Summary\n"
        << "================================================================================\n"
        << "Design Information:\n"
        << "  Module Name:                 " << circuit.module_name << "\n"
        << "  Total Nets:                  " << circuit.nets.size() << "\n"
        << "  Primary Inputs:              " << circuit.primary_inputs.size() << "\n"
        << "  Primary Outputs:             " << circuit.primary_outputs.size() << "\n"
        << "  Logic Gates:                 " << circuit.gates.size() << "\n\n"
        << "Simulation Parameters:\n"
        << "  Vectors:                     " << sim_result.num_vectors << "\n"
        << "  Measured Intervals:          " << sim_result.measured_intervals << "\n"
        << "  Random Seed:                 " << sim_result.seed << "\n\n"
        << "Electrical Parameters:\n"
        << "  Operating Voltage:           " << std::fixed << std::setprecision(6) << power_summary.voltage_V << " V\n"
        << "  Operating Frequency:         " << std::fixed << std::setprecision(6) << power_summary.frequency_Hz << " Hz\n"
        << "  Default Capacitance:         " << std::fixed << std::setprecision(4) << power_summary.default_capacitance_fF << " fF\n"
        << "  Capacitance Overrides:       " << power_summary.overridden_capacitance_count << "\n\n"
        << "Estimated Power Results:\n"
        << "  Total Dynamic Power:         " << std::fixed << std::setprecision(6) << power_summary.total_power_nW << " nW\n"
        << "  Total Dynamic Power (Watts): " << std::scientific << std::setprecision(6) << power_summary.total_power_W << " W\n\n"
        << "Model & Methodology Notes:\n"
        << "  Power Model:                 P_dynamic = 0.5 * alpha * C * V^2 * f\n"
        << "  Alpha Convention:            Total (0->1 + 1->0) transitions per net per interval\n"
        << "  Capacitance:                 Converted to Farads (1 fF = 1e-15 F)\n"
        << "  Simulation:                  Zero-delay combinational simulation\n\n"
        << "Limitations:\n"
        << "  - Educational estimator; NOT for sign-off power analysis.\n"
        << "  - Zero-delay simulation does not model glitches, hazards, or transport delays.\n"
        << "  - Does not model leakage power, internal cell power, or short-circuit current.\n"
        << "  - Does not model sequential elements, clock trees, or wire layout parasitics.\n"
        << "================================================================================\n";
    return oss.str();
}

static std::string build_csv_text(const Circuit& circuit) {
    std::ostringstream oss;
    oss << "net,role,transitions,rising_transitions,falling_transitions,alpha,capacitance_fF,power_nW\n";

    // Sort nets alphabetically by name for deterministic CSV output
    std::vector<const Net*> sorted_nets;
    sorted_nets.reserve(circuit.nets.size());
    for (const auto& net : circuit.nets) {
        sorted_nets.push_back(&net);
    }
    std::sort(sorted_nets.begin(), sorted_nets.end(), [](const Net* a, const Net* b) {
        return a->name < b->name;
    });

    for (const Net* net : sorted_nets) {
        oss << net->name << ","
            << net_role_to_string(net->role) << ","
            << net->total_transitions << ","
            << net->rising_transitions << ","
            << net->falling_transitions << ","
            << std::fixed << std::setprecision(6) << net->alpha << ","
            << std::fixed << std::setprecision(4) << net->capacitance_fF << ","
            << std::fixed << std::setprecision(6) << net->power_nW << "\n";
    }

    return oss.str();
}

void write_reports(const std::string& output_dir,
                   const Circuit& circuit,
                   const SimulationResult& sim_result,
                   const PowerSummary& power_summary) {
    fs::create_directories(output_dir);

    fs::path dir_path(output_dir);

    // 1. summary.txt
    {
        fs::path summary_path = dir_path / "summary.txt";
        std::ofstream ofs(summary_path);
        if (!ofs.is_open()) {
            throw std::runtime_error("failed to open output summary file: " + summary_path.string());
        }
        ofs << build_summary_text(circuit, sim_result, power_summary);
    }

    // 2. net_activity.csv
    {
        fs::path csv_path = dir_path / "net_activity.csv";
        std::ofstream ofs(csv_path);
        if (!ofs.is_open()) {
            throw std::runtime_error("failed to open output CSV file: " + csv_path.string());
        }
        ofs << build_csv_text(circuit);
    }
}

void print_stdout_summary(const Circuit& circuit,
                          const SimulationResult& sim_result,
                          const PowerSummary& power_summary) {
    std::cout << build_summary_text(circuit, sim_result, power_summary);
}

}  // namespace netpower

