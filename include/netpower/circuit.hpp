#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace netpower {

enum class GateType {
    AND,
    OR,
    NOT,
    NAND,
    NOR,
    XOR,
    XNOR,
    BUF
};

std::string_view gate_type_to_string(GateType type);
std::optional<GateType> string_to_gate_type(std::string_view str);
bool is_valid_gate_arity(GateType type, size_t input_count);

enum class NetRole {
    INPUT,
    INTERNAL,
    OUTPUT,
    INPUT_OUTPUT  // Direct primary input to primary output alias
};

std::string_view net_role_to_string(NetRole role);

enum class InputKind {
    NET,
    CONST_0,
    CONST_1
};

struct GateInput {
    InputKind kind{InputKind::NET};
    size_t net_index{0};
    std::string name; // For diagnostic/debugging
};

struct Gate {
    std::string instance_name;
    GateType type{GateType::BUF};
    size_t output_net_index{0};
    std::vector<GateInput> inputs;
    size_t original_index{0};
    size_t line{0};
    size_t col{0};
};

struct Net {
    std::string name;
    NetRole role{NetRole::INTERNAL};
    std::optional<size_t> driver_gate_index;
    std::vector<size_t> fanout_gate_indices;

    // Simulation & activity
    uint64_t total_transitions{0};
    uint64_t rising_transitions{0};
    uint64_t falling_transitions{0};
    bool current_value{false};

    // Power & Capacitance
    double capacitance_fF{0.0};
    double power_nW{0.0};
    double alpha{0.0};

    // Source declaration location
    size_t line{0};
    size_t col{0};
};

struct Circuit {
    std::string module_name;
    std::vector<Net> nets;
    std::unordered_map<std::string, size_t> net_name_to_index;
    std::vector<Gate> gates;
    std::unordered_map<std::string, size_t> gate_name_to_index;

    std::vector<size_t> primary_inputs;
    std::vector<size_t> primary_outputs;

    // Precomputed Kahn topological evaluation order
    std::vector<size_t> topological_order;

    [[nodiscard]] std::optional<size_t> find_net_index(std::string_view name) const;
    [[nodiscard]] std::optional<size_t> find_gate_index(std::string_view name) const;
};

// Evaluate gate output given boolean input values
bool evaluate_gate(GateType type, const std::vector<bool>& inputs);

}  // namespace netpower

