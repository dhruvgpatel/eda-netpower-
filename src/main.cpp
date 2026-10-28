#include "netpower/circuit.hpp"
#include "netpower/cli.hpp"
#include "netpower/graph.hpp"
#include "netpower/parser.hpp"
#include "netpower/power.hpp"
#include "netpower/report.hpp"
#include "netpower/simulator.hpp"

#include <iostream>

int main(int argc, char* argv[]) {
    try {
        netpower::CliOptions opts = netpower::CliParser::parse(argc, argv);
        if (opts.help_requested) {
            netpower::CliParser::print_help(argv[0]);
            return 0;
        }

        // 1. Parse Verilog netlist
        netpower::Circuit circuit = netpower::Parser::parse_file(opts.netlist_file);

        // 2. Validate connectivity and compute topological order
        netpower::validate_and_order_circuit(circuit);

        // 3. Simulate random vectors
        netpower::SimulationConfig sim_cfg;
        sim_cfg.num_vectors = opts.vectors;
        sim_cfg.seed = opts.seed;
        netpower::Simulator simulator(circuit, sim_cfg);
        netpower::SimulationResult sim_result = simulator.run();

        // 4. Calculate dynamic power
        netpower::PowerConfig power_cfg;
        power_cfg.voltage_V = opts.voltage_V;
        power_cfg.frequency_Hz = opts.frequency_Hz;
        power_cfg.default_capacitance_fF = opts.default_capacitance_fF;
        power_cfg.capacitance_file = opts.capacitance_file;
        netpower::PowerSummary power_summary = netpower::calculate_power(circuit, power_cfg);

        // 5. Generate reports
        netpower::write_reports(opts.output_dir, circuit, sim_result, power_summary);

        // 6. Print summary to stdout
        netpower::print_stdout_summary(circuit, sim_result, power_summary);

        return 0;
    } catch (const netpower::ParseError& e) {
        std::cerr << "Parser error: " << e.what() << "\n";
        return 1;
    } catch (const netpower::GraphError& e) {
        std::cerr << "Graph error: " << e.what() << "\n";
        return 1;
    } catch (const netpower::PowerModelError& e) {
        std::cerr << "Power model error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}

