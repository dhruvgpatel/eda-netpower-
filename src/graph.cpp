#include "netpower/graph.hpp"

#include <algorithm>
#include <queue>
#include <sstream>
#include <unordered_set>

namespace netpower {

DependencyGraph build_dependency_graph(const Circuit& circuit) {
    DependencyGraph graph;
    graph.num_gates = circuit.gates.size();
    graph.adj.resize(graph.num_gates);
    graph.in_degree.assign(graph.num_gates, 0);

    for (size_t consumer_idx = 0; consumer_idx < circuit.gates.size(); ++consumer_idx) {
        const Gate& consumer = circuit.gates[consumer_idx];
        std::unordered_set<size_t> predecessor_gates;

        for (const auto& gin : consumer.inputs) {
            if (gin.kind == InputKind::NET) {
                const Net& src_net = circuit.nets[gin.net_index];
                if (src_net.driver_gate_index.has_value()) {
                    predecessor_gates.insert(*src_net.driver_gate_index);
                }
            }
        }

        graph.in_degree[consumer_idx] = predecessor_gates.size();
        for (size_t pred_idx : predecessor_gates) {
            graph.adj[pred_idx].push_back(consumer_idx);
        }
    }

    return graph;
}

std::vector<size_t> compute_topological_order(const Circuit& circuit, const DependencyGraph& graph) {
    std::vector<size_t> in_deg = graph.in_degree;
    std::priority_queue<size_t, std::vector<size_t>, std::greater<size_t>> ready_queue;

    for (size_t i = 0; i < graph.num_gates; ++i) {
        if (in_deg[i] == 0) {
            ready_queue.push(i);
        }
    }

    std::vector<size_t> order;
    order.reserve(graph.num_gates);

    while (!ready_queue.empty()) {
        size_t u = ready_queue.top();
        ready_queue.pop();
        order.push_back(u);

        for (size_t v : graph.adj[u]) {
            if (--in_deg[v] == 0) {
                ready_queue.push(v);
            }
        }
    }

    if (order.size() < graph.num_gates) {
        // Combinational cycle detected
        std::vector<std::string> blocked_gates;
        for (size_t i = 0; i < graph.num_gates; ++i) {
            if (in_deg[i] > 0) {
                blocked_gates.push_back(circuit.gates[i].instance_name);
            }
        }
        std::sort(blocked_gates.begin(), blocked_gates.end());

        std::ostringstream oss;
        oss << "combinational loop detected: " << blocked_gates.size()
            << " gate(s) blocked in cycle: [";
        for (size_t i = 0; i < blocked_gates.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << "'" << blocked_gates[i] << "'";
        }
        oss << "]";
        throw GraphError(oss.str());
    }

    return order;
}

void validate_and_order_circuit(Circuit& circuit) {
    DependencyGraph graph = build_dependency_graph(circuit);
    circuit.topological_order = compute_topological_order(circuit, graph);
}

}  // namespace netpower

