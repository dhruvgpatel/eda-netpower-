#pragma once

#include "netpower/circuit.hpp"
#include "netpower/power.hpp"
#include "netpower/simulator.hpp"

#include <string>

namespace netpower {

void write_reports(const std::string& output_dir,
                   const Circuit& circuit,
                   const SimulationResult& sim_result,
                   const PowerSummary& power_summary);

void print_stdout_summary(const Circuit& circuit,
                          const SimulationResult& sim_result,
                          const PowerSummary& power_summary);

}  // namespace netpower

