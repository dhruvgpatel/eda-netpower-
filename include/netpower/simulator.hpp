#pragma once

#include "netpower/circuit.hpp"

#include <cstdint>
#include <random>

namespace netpower {

struct SimulationConfig {
    uint64_t num_vectors{1000};
    uint64_t seed{42};
};

struct SimulationResult {
    uint64_t num_vectors{0};
    uint64_t measured_intervals{0};
    uint64_t seed{0};
};

class Simulator {
public:
    explicit Simulator(Circuit& circuit, const SimulationConfig& config);

    SimulationResult run();

private:
    Circuit& circuit_;
    SimulationConfig config_;
    std::mt19937_64 rng_;
};

}  // namespace netpower

