#include "netpower/circuit.hpp"
#include "netpower/cli.hpp"
#include "netpower/graph.hpp"
#include "netpower/parser.hpp"
#include "netpower/power.hpp"
#include "netpower/simulator.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static int g_failures = 0;
static int g_total = 0;

#define TEST_ASSERT(cond)                                                                \
    do {                                                                                 \
        if (!(cond)) {                                                                   \
            std::cerr << "  Assertion failed at " << __FILE__ << ":" << __LINE__        \
                      << ": " #cond "\n";                                                \
            return false;                                                                \
        }                                                                                \
    } while (0)

#define TEST_ASSERT_THROWS(expr, ExceptionType)                                          \
    do {                                                                                 \
        bool caught = false;                                                             \
        try {                                                                            \
            expr;                                                                        \
        } catch (const ExceptionType&) {                                                 \
            caught = true;                                                               \
        } catch (...) {}                                                                 \
        if (!caught) {                                                                   \
            std::cerr << "  Expected " #ExceptionType " at " << __FILE__ << ":"          \
                      << __LINE__ << " for " #expr "\n";                                 \
            return false;                                                                \
        }                                                                                \
    } while (0)

static void run_test(const std::string& name, bool (*test_fn)()) {
    g_total++;
    std::cout << "[RUN] " << name << " ... ";
    if (test_fn()) {
        std::cout << "PASSED\n";
    } else {
        std::cout << "FAILED\n";
        g_failures++;
    }
}

// 1. Truth table tests
bool test_truth_tables() {
    // BUF
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::BUF, {false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::BUF, {true}) == true);

    // NOT
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NOT, {false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NOT, {true}) == false);

    // AND
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::AND, {false, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::AND, {false, true}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::AND, {true, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::AND, {true, true}) == true);

    // NAND
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NAND, {false, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NAND, {true, true}) == false);

    // OR
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::OR, {false, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::OR, {true, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::OR, {true, true}) == true);

    // NOR
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NOR, {false, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::NOR, {false, true}) == false);

    // XOR (2 inputs)
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {false, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {false, true}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {true, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {true, true}) == false);

    // XNOR (2 inputs)
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {false, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {false, true}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {true, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {true, true}) == true);

    // Multi-input XOR / XNOR
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {false, false, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {true, false, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {true, true, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XOR, {true, true, true}) == true);

    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {false, false, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {true, false, false}) == false);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {true, true, false}) == true);
    TEST_ASSERT(netpower::evaluate_gate(netpower::GateType::XNOR, {true, true, true}) == false);

    return true;
}

// 2. Parser valid circuits & comments
bool test_parser_valid() {
    std::string verilog = R"(
        // Test module with comments and whitespace
        /* Multi-line
           comment */
        module test_circ(in1, in2, out1, out2);
            input in1, in2;
            output out1, out2;
            wire w1;

            and G1 (w1, in1, 1'b1);
            or  G2 (out1, w1, 1'b0);
            not G3 (out2, in2);
        endmodule
    )";

    netpower::Circuit c = netpower::Parser::parse_string(verilog, "test_circ.v");
    TEST_ASSERT(c.module_name == "test_circ");
    TEST_ASSERT(c.primary_inputs.size() == 2);
    TEST_ASSERT(c.primary_outputs.size() == 2);
    TEST_ASSERT(c.gates.size() == 3);

    // Constant inputs
    TEST_ASSERT(c.gates[0].inputs[1].kind == netpower::InputKind::CONST_1);
    TEST_ASSERT(c.gates[1].inputs[1].kind == netpower::InputKind::CONST_0);

    return true;
}

// 3. Direct primary-input to primary-output alias
bool test_direct_alias() {
    std::string verilog = R"(
        module passthrough(a);
            input a;
            output a;
        endmodule
    )";

    netpower::Circuit c = netpower::Parser::parse_string(verilog, "passthrough.v");
    TEST_ASSERT(c.nets.size() == 1);
    TEST_ASSERT(c.nets[0].name == "a");
    TEST_ASSERT(c.nets[0].role == netpower::NetRole::INPUT_OUTPUT);
    TEST_ASSERT(c.gates.empty());

    netpower::validate_and_order_circuit(c);
    TEST_ASSERT(c.topological_order.empty());

    netpower::SimulationConfig cfg;
    cfg.num_vectors = 100;
    cfg.seed = 42;
    netpower::Simulator sim(c, cfg);
    netpower::SimulationResult res = sim.run();
    TEST_ASSERT(res.measured_intervals == 99);
    TEST_ASSERT(c.nets[0].total_transitions > 0);

    return true;
}

// 4. Parser errors
bool test_parser_errors() {
    // Undeclared signal
    {
        std::string v = "module m(a, y); input a; output y; and G1(y, a, undeclared_net); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Duplicate declaration
    {
        std::string v = "module m(a, y); input a; input a; output y; buf G1(y, a); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Malformed syntax (missing semicolon)
    {
        std::string v = "module m(a, y) input a; output y; endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Invalid gate arity for NOT (expects 1 input)
    {
        std::string v = "module m(a, b, y); input a, b; output y; not G1(y, a, b); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Invalid gate arity for AND (expects >= 2 inputs)
    {
        std::string v = "module m(a, y); input a; output y; and G1(y, a); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Unsupported construct: assign
    {
        std::string v = "module m(a, y); input a; output y; assign y = a; endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Unsupported construct: reg
    {
        std::string v = "module m(a, y); input a; output y; reg r; endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Multiple modules
    {
        std::string v = "module m1(a); input a; endmodule module m2(b); input b; endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Malformed constant
    {
        std::string v = "module m(a, y); input a; output y; and G1(y, a, 1'b2); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Primary input driven by gate
    {
        std::string v = "module m(a, b); input a, b; and G1(a, b, 1'b1); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    // Multiple drivers for net
    {
        std::string v = "module m(a, b, y); input a, b; output y; and G1(y, a, b); or G2(y, a, b); endmodule";
        TEST_ASSERT_THROWS(netpower::Parser::parse_string(v), netpower::ParseError);
    }
    return true;
}

// 5. Dependency graph and loop detection
bool test_graph_and_loops() {
    // Kahn order on a chain: a -> G1(w1) -> G2(w2) -> G3(y)
    {
        std::string v = R"(
            module chain(a, y);
                input a;
                output y;
                wire w1, w2;
                buf G2 (w2, w1);
                buf G3 (y, w2);
                buf G1 (w1, a);
            endmodule
        )";
        netpower::Circuit c = netpower::Parser::parse_string(v);
        netpower::validate_and_order_circuit(c);
        TEST_ASSERT(c.topological_order.size() == 3);
        // G1 must come before G2, and G2 before G3
        size_t g1_idx = *c.find_gate_index("G1");
        size_t g2_idx = *c.find_gate_index("G2");
        size_t g3_idx = *c.find_gate_index("G3");

        size_t pos_g1 = 0, pos_g2 = 0, pos_g3 = 0;
        for (size_t i = 0; i < c.topological_order.size(); ++i) {
            if (c.topological_order[i] == g1_idx) pos_g1 = i;
            if (c.topological_order[i] == g2_idx) pos_g2 = i;
            if (c.topological_order[i] == g3_idx) pos_g3 = i;
        }
        TEST_ASSERT(pos_g1 < pos_g2);
        TEST_ASSERT(pos_g2 < pos_g3);
    }

    // Combinational cycle
    {
        std::string v = R"(
            module loop(a, y);
                input a;
                output y;
                wire w1, w2;
                and G1 (w1, a, w2);
                or  G2 (w2, w1, a);
                buf G3 (y, w2);
            endmodule
        )";
        netpower::Circuit c = netpower::Parser::parse_string(v);
        bool caught = false;
        try {
            netpower::validate_and_order_circuit(c);
        } catch (const netpower::GraphError& ge) {
            caught = true;
            std::string msg = ge.what();
            // Should mention blocked gates G1 and G2
            TEST_ASSERT(msg.find("G1") != std::string::npos);
            TEST_ASSERT(msg.find("G2") != std::string::npos);
        }
        TEST_ASSERT(caught);
    }

    // Self loop
    {
        std::string v = R"(
            module self_loop(a, y);
                input a;
                output y;
                wire w;
                and G1 (w, a, w);
                buf G2 (y, w);
            endmodule
        )";
        netpower::Circuit c = netpower::Parser::parse_string(v);
        TEST_ASSERT_THROWS(netpower::validate_and_order_circuit(c), netpower::GraphError);
    }

    return true;
}

// 6. Simulation reproducibility & transition accounting
bool test_simulation_reproducibility() {
    std::string v = R"(
        module test_sim(a, b, sum, carry);
            input a, b;
            output sum, carry;
            xor G1 (sum, a, b);
            and G2 (carry, a, b);
        endmodule
    )";

    netpower::Circuit c1 = netpower::Parser::parse_string(v);
    netpower::validate_and_order_circuit(c1);

    netpower::SimulationConfig cfg;
    cfg.num_vectors = 5000;
    cfg.seed = 12345;

    netpower::Simulator sim1(c1, cfg);
    sim1.run();

    netpower::Circuit c2 = netpower::Parser::parse_string(v);
    netpower::validate_and_order_circuit(c2);
    netpower::Simulator sim2(c2, cfg);
    sim2.run();

    // Verify exact reproducibility
    for (size_t i = 0; i < c1.nets.size(); ++i) {
        TEST_ASSERT(c1.nets[i].total_transitions == c2.nets[i].total_transitions);
        TEST_ASSERT(c1.nets[i].rising_transitions == c2.nets[i].rising_transitions);
        TEST_ASSERT(c1.nets[i].falling_transitions == c2.nets[i].falling_transitions);
        TEST_ASSERT(c1.nets[i].alpha == c2.nets[i].alpha);
        // Verify rising + falling == total
        TEST_ASSERT(c1.nets[i].rising_transitions + c1.nets[i].falling_transitions ==
                    c1.nets[i].total_transitions);
        // Verify alpha = total / 4999
        double expected_alpha = static_cast<double>(c1.nets[i].total_transitions) / 4999.0;
        TEST_ASSERT(std::abs(c1.nets[i].alpha - expected_alpha) < 1e-12);
    }

    return true;
}

// 7. Power scaling: V^2, f, C
bool test_power_scaling() {
    std::string v = R"(
        module adder(a, b, sum);
            input a, b;
            output sum;
            xor G1 (sum, a, b);
        endmodule
    )";

    netpower::Circuit c = netpower::Parser::parse_string(v);
    netpower::validate_and_order_circuit(c);

    netpower::SimulationConfig sim_cfg;
    sim_cfg.num_vectors = 2000;
    sim_cfg.seed = 42;
    netpower::Simulator sim(c, sim_cfg);
    sim.run();

    netpower::PowerConfig p_base;
    p_base.voltage_V = 1.0;
    p_base.frequency_Hz = 1e8;
    p_base.default_capacitance_fF = 2.0;

    netpower::PowerSummary s_base = netpower::calculate_power(c, p_base);
    TEST_ASSERT(s_base.total_power_nW > 0.0);

    // Doubling voltage quadruples power (V^2)
    netpower::PowerConfig p_2v = p_base;
    p_2v.voltage_V = 2.0;
    netpower::PowerSummary s_2v = netpower::calculate_power(c, p_2v);
    double ratio_v = s_2v.total_power_nW / s_base.total_power_nW;
    TEST_ASSERT(std::abs(ratio_v - 4.0) < 1e-6);

    // Doubling frequency doubles power
    netpower::PowerConfig p_2f = p_base;
    p_2f.frequency_Hz = 2e8;
    netpower::PowerSummary s_2f = netpower::calculate_power(c, p_2f);
    double ratio_f = s_2f.total_power_nW / s_base.total_power_nW;
    TEST_ASSERT(std::abs(ratio_f - 2.0) < 1e-6);

    // Doubling capacitance doubles power
    netpower::PowerConfig p_2c = p_base;
    p_2c.default_capacitance_fF = 4.0;
    netpower::PowerSummary s_2c = netpower::calculate_power(c, p_2c);
    double ratio_c = s_2c.total_power_nW / s_base.total_power_nW;
    TEST_ASSERT(std::abs(ratio_c - 2.0) < 1e-6);

    // Net with zero transitions gives zero power
    for (auto& net : c.nets) {
        net.alpha = 0.0;
    }
    netpower::PowerSummary s_zero = netpower::calculate_power(c, p_base);
    TEST_ASSERT(s_zero.total_power_nW == 0.0);

    return true;
}

// 8. CLI options validation
bool test_cli_parsing() {
    // Valid options
    {
        const char* argv[] = {
            "netpower",
            "--netlist", "test.v",
            "--vectors", "1000",
            "--seed", "42",
            "--voltage", "1.1",
            "--frequency-hz", "50000000",
            "--default-capacitance-ff", "1.5",
            "--output-dir", "out_dir"
        };
        int argc = sizeof(argv) / sizeof(argv[0]);
        netpower::CliOptions opts = netpower::CliParser::parse(argc, const_cast<char**>(argv));
        TEST_ASSERT(opts.netlist_file == "test.v");
        TEST_ASSERT(opts.vectors == 1000);
        TEST_ASSERT(opts.seed == 42);
        TEST_ASSERT(opts.voltage_V == 1.1);
        TEST_ASSERT(opts.frequency_Hz == 50000000.0);
        TEST_ASSERT(opts.default_capacitance_fF == 1.5);
        TEST_ASSERT(opts.output_dir == "out_dir");
    }

    // Missing required option
    {
        const char* argv[] = {"netpower", "--vectors", "1000"};
        int argc = sizeof(argv) / sizeof(argv[0]);
        TEST_ASSERT_THROWS(netpower::CliParser::parse(argc, const_cast<char**>(argv)), std::runtime_error);
    }

    // Vectors < 2
    {
        const char* argv[] = {
            "netpower", "--netlist", "t.v", "--vectors", "1", "--seed", "1",
            "--voltage", "1.0", "--frequency-hz", "100", "--default-capacitance-ff", "1.0",
            "--output-dir", "out"
        };
        int argc = sizeof(argv) / sizeof(argv[0]);
        TEST_ASSERT_THROWS(netpower::CliParser::parse(argc, const_cast<char**>(argv)), std::runtime_error);
    }

    // Negative voltage
    {
        const char* argv[] = {
            "netpower", "--netlist", "t.v", "--vectors", "10", "--seed", "1",
            "--voltage", "-1.0", "--frequency-hz", "100", "--default-capacitance-ff", "1.0",
            "--output-dir", "out"
        };
        int argc = sizeof(argv) / sizeof(argv[0]);
        TEST_ASSERT_THROWS(netpower::CliParser::parse(argc, const_cast<char**>(argv)), std::runtime_error);
    }

    // Unknown option
    {
        const char* argv[] = {"netpower", "--unknown"};
        int argc = sizeof(argv) / sizeof(argv[0]);
        TEST_ASSERT_THROWS(netpower::CliParser::parse(argc, const_cast<char**>(argv)), std::runtime_error);
    }

    return true;
}

int main() {
    std::cout << "Running Netpower Unit Tests...\n";
    run_test("Truth Tables", test_truth_tables);
    run_test("Parser Valid Circuits & Comments", test_parser_valid);
    run_test("Direct Alias", test_direct_alias);
    run_test("Parser Errors", test_parser_errors);
    run_test("Graph & Loops", test_graph_and_loops);
    run_test("Simulation Reproducibility", test_simulation_reproducibility);
    run_test("Power Scaling", test_power_scaling);
    run_test("CLI Parsing", test_cli_parsing);

    std::cout << "\nTest Summary: " << (g_total - g_failures) << "/" << g_total
              << " passed. (" << g_failures << " failures)\n";
    return g_failures == 0 ? 0 : 1;
}

