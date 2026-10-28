#pragma once

#include "netpower/circuit.hpp"

#include <stdexcept>
#include <string>
#include <vector>

namespace netpower {

class GraphError : public std::runtime_error {
public:
    explicit GraphError(const std::string& message) : std::runtime_error(message) {}
};

struct DependencyGraph {
    size_t num_gates{0};
    std::vector<std::vector<size_t>> adj;
    std::vector<size_t> in_degree;
};

// Builds gate dependency graph with deduplicated edges between gates
DependencyGraph build_dependency_graph(const Circuit& circuit);

// Deterministic Kahn's algorithm topological sort.
// Throws GraphError if a cycle is detected, listing the blocked gates.
std::vector<size_t> compute_topological_order(const Circuit& circuit, const DependencyGraph& graph);

// Validates circuit and populates circuit.topological_order
void validate_and_order_circuit(Circuit& circuit);

}  // namespace netpower

