#include "netpower/simulator.hpp"

#include <stdexcept>
#include <vector>

namespace netpower {

Simulator::Simulator(Circuit& circuit, const SimulationConfig& config)
    : circuit_(circuit), config_(config), rng_(config.seed) {
    if (config_.num_vectors < 2) {
        throw std::invalid_argument("number of vectors must be at least 2");
    }
}

SimulationResult Simulator::run() {
    std::bernoulli_distribution dist(0.5);

    const size_t num_nets = circuit_.nets.size();
    std::vector<bool> prev_values(num_nets, false);

    // Reset transitions and alpha
    for (auto& net : circuit_.nets) {
        net.total_transitions = 0;
        net.rising_transitions = 0;
        net.falling_transitions = 0;
        net.alpha = 0.0;
    }

    const uint64_t num_vectors = config_.num_vectors;
    const uint64_t measured_intervals = num_vectors - 1;

    for (uint64_t vec = 0; vec < num_vectors; ++vec) {
        // 1. Assign primary inputs
        for (size_t pi_idx : circuit_.primary_inputs) {
            circuit_.nets[pi_idx].current_value = dist(rng_);
        }

        // 2. Evaluate gates in precomputed topological order
        for (size_t gate_idx : circuit_.topological_order) {
            const Gate& gate = circuit_.gates[gate_idx];
            std::vector<bool> gate_inputs;
            gate_inputs.reserve(gate.inputs.size());

            for (const auto& gin : gate.inputs) {
                if (gin.kind == InputKind::CONST_0) {
                    gate_inputs.push_back(false);
                } else if (gin.kind == InputKind::CONST_1) {
                    gate_inputs.push_back(true);
                } else {
                    gate_inputs.push_back(circuit_.nets[gin.net_index].current_value);
                }
            }

            bool out_val = evaluate_gate(gate.type, gate_inputs);
            circuit_.nets[gate.output_net_index].current_value = out_val;
        }

        // 3. Compare with previous vector
        if (vec == 0) {
            for (size_t i = 0; i < num_nets; ++i) {
                prev_values[i] = circuit_.nets[i].current_value;
            }
        } else {
            for (size_t i = 0; i < num_nets; ++i) {
                bool curr = circuit_.nets[i].current_value;
                bool prev = prev_values[i];
                if (!prev && curr) {
                    circuit_.nets[i].rising_transitions++;
                    circuit_.nets[i].total_transitions++;
                } else if (prev && !curr) {
                    circuit_.nets[i].falling_transitions++;
                    circuit_.nets[i].total_transitions++;
                }
                prev_values[i] = curr;
            }
        }
    }

    // Compute alpha for every net
    for (auto& net : circuit_.nets) {
        net.alpha = static_cast<double>(net.total_transitions) / static_cast<double>(measured_intervals);
    }

    SimulationResult result;
    result.num_vectors = num_vectors;
    result.measured_intervals = measured_intervals;
    result.seed = config_.seed;
    return result;
}

}  // namespace netpower

