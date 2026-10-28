#include "netpower/circuit.hpp"

#include <numeric>

namespace netpower {

std::string_view gate_type_to_string(GateType type) {
    switch (type) {
        case GateType::AND:  return "and";
        case GateType::OR:   return "or";
        case GateType::NOT:  return "not";
        case GateType::NAND: return "nand";
        case GateType::NOR:  return "nor";
        case GateType::XOR:  return "xor";
        case GateType::XNOR: return "xnor";
        case GateType::BUF:  return "buf";
    }
    return "unknown";
}

std::optional<GateType> string_to_gate_type(std::string_view str) {
    if (str == "and")  return GateType::AND;
    if (str == "or")   return GateType::OR;
    if (str == "not")  return GateType::NOT;
    if (str == "nand") return GateType::NAND;
    if (str == "nor")  return GateType::NOR;
    if (str == "xor")  return GateType::XOR;
    if (str == "xnor") return GateType::XNOR;
    if (str == "buf")  return GateType::BUF;
    return std::nullopt;
}

bool is_valid_gate_arity(GateType type, size_t input_count) {
    switch (type) {
        case GateType::NOT:
        case GateType::BUF:
            return input_count == 1;
        case GateType::AND:
        case GateType::OR:
        case GateType::NAND:
        case GateType::NOR:
        case GateType::XOR:
        case GateType::XNOR:
            return input_count >= 2;
    }
    return false;
}

std::string_view net_role_to_string(NetRole role) {
    switch (role) {
        case NetRole::INPUT:        return "input";
        case NetRole::INTERNAL:     return "internal";
        case NetRole::OUTPUT:       return "output";
        case NetRole::INPUT_OUTPUT: return "input_output";
    }
    return "internal";
}

std::optional<size_t> Circuit::find_net_index(std::string_view name) const {
    auto it = net_name_to_index.find(std::string(name));
    if (it != net_name_to_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::optional<size_t> Circuit::find_gate_index(std::string_view name) const {
    auto it = gate_name_to_index.find(std::string(name));
    if (it != gate_name_to_index.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool evaluate_gate(GateType type, const std::vector<bool>& inputs) {
    switch (type) {
        case GateType::BUF:
            return inputs.empty() ? false : inputs[0];
        case GateType::NOT:
            return inputs.empty() ? true : !inputs[0];
        case GateType::AND: {
            for (bool val : inputs) {
                if (!val) return false;
            }
            return true;
        }
        case GateType::NAND: {
            for (bool val : inputs) {
                if (!val) return true;
            }
            return false;
        }
        case GateType::OR: {
            for (bool val : inputs) {
                if (val) return true;
            }
            return false;
        }
        case GateType::NOR: {
            for (bool val : inputs) {
                if (val) return false;
            }
            return true;
        }
        case GateType::XOR: {
            // Odd parity: true if odd number of inputs are true
            size_t count_ones = 0;
            for (bool val : inputs) {
                if (val) count_ones++;
            }
            return (count_ones % 2) != 0;
        }
        case GateType::XNOR: {
            // Complement of XOR: true if even number of inputs are true
            size_t count_ones = 0;
            for (bool val : inputs) {
                if (val) count_ones++;
            }
            return (count_ones % 2) == 0;
        }
    }
    return false;
}

}  // namespace netpower

