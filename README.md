# netpower: Gate-Level Netlist Parser & Dynamic Power Estimator

`netpower` is a lightweight, dependency-free C++17 command-line EDA application for parsing structural Verilog gate-level netlists, computing deterministic topological evaluation orders, simulating random input vectors, and estimating dynamic switching power.

> **Disclaimer**: `netpower` is an educational, zero-delay dynamic power estimator. It is **not** a sign-off power analysis tool (e.g. PrimeTime PX). It does not model internal cell power, static leakage, short-circuit current, glitching/hazards, sequential timing, clock trees, or characterized Liberty library tables.

---

## Features

- **Structural Verilog Lexer & Parser**: Token-based lexer and recursive-descent parser tracking line and column numbers. Detailed diagnostics for syntax and semantic violations.
- **Supported Gate Primitives**: `and`, `or`, `not`, `nand`, `nor`, `xor`, `xnor`, and `buf`. Correct multi-input parity semantics for `xor` and `xnor`. Constant inputs `1'b0` and `1'b1`.
- **Topological Sorting & Cycle Detection**: Deterministic Kahn's algorithm using a min-priority queue. Identifies combinational loops and lists blocked gates. Edge deduplication for multi-fanin nets.
- **Reproducible Vector Simulation**: Seeded pseudo-random vector generation using `std::mt19937_64` with independent Bernoulli(0.5) input distributions.
- **Accurate Transition Accounting**: Distinguishes rising ($0 \to 1$) and falling ($1 \to 0$) transitions. Startup vector excluded from transition count.
- **Explicit Dynamic Power Model**: $P = 0.5 \times \alpha \times C \times V^2 \times f$, with $\alpha$ explicitly defined as total $(0 \to 1) + (1 \to 0)$ transitions per net per interval.
- **Per-Net Capacitance Overrides**: Default capacitance in femtofarads (fF) with optional CSV overrides.
- **Deterministic Reports**: Stable `summary.txt` and `net_activity.csv` (sorted by net name) without timestamps or environment-specific paths.
- **Zero Third-Party Dependencies**: Pure C++17 standard library, CMake, CTest, and Bash automation.

---

## Supported Netlist Grammar

The parser supports a clearly defined scalar structural Verilog subset:
- Non-ANSI module header: `module <name>(port1, port2, ...);`
- Port declarations: `input a, b;`, `output sum, carry;`, and `wire n1, n2;`
- Gate primitive instantiations: `<primitive> <inst_name> (<out_net>, <in_net1>, <in_net2>, ...);`
- Literal constants: `1'b0` and `1'b1`
- Single-line (`//`) and multi-line (`/* ... */`) comments

Unsupported constructs (e.g. `assign`, `always`, `reg`, vectors/buses, hierarchy, delays) are rejected with clear line and column error messages.

### Example: Half Adder (`examples/half_adder.v`)

```verilog
module half_adder(a, b, sum, carry);
  input a, b;
  output sum, carry;
  xor G1 (sum, a, b);
  and G2 (carry, a, b);
endmodule
```

---

## Build Instructions

### Prerequisites (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build clang git
```

### Build with CMake

```bash
# Debug build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel

# Release build
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-release --parallel
```

Executables are placed consistently in `${BUILD_DIR}/bin/netpower`.

---

## Running netpower

```bash
./build/bin/netpower \
  --netlist examples/half_adder.v \
  --vectors 10000 \
  --seed 42 \
  --voltage 1.0 \
  --frequency-hz 100000000 \
  --default-capacitance-ff 2.0 \
  --output-dir results/run-001
```

### Command-Line Arguments

| Option | Required | Description |
|---|---|---|
| `--netlist <path>` | Yes | Path to structural Verilog netlist file |
| `--vectors <count>` | Yes | Number of simulation vectors ($N \ge 2$) |
| `--seed <int>` | Yes | 64-bit integer seed for PRNG reproducibility |
| `--voltage <volts>` | Yes | Operating voltage in Volts ($V > 0$) |
| `--frequency-hz <Hz>` | Yes | Operating frequency in Hertz ($f > 0$) |
| `--default-capacitance-ff <fF>` | Yes | Default net capacitance in femtofarads ($C \ge 0$) |
| `--output-dir <dir>` | Yes | Directory where report files are saved |
| `--capacitance-file <path>` | No | CSV file with per-net overrides (`net,capacitance_fF`) |
| `-h, --help` | No | Show help message and exit |

---

## Methodology & Theoretical Foundations

### 1. Topological Sorting (Kahn's Algorithm)
A gate dependency graph is constructed where directed edge $G_A \to G_B$ indicates gate $G_A$ drives a net that is an input to gate $G_B$. Edges are deduplicated so that multiple connections between the same gates do not artificially inflate in-degrees.
Topological order is resolved deterministically using a min-priority queue keyed by original gate index:
- If all gates are processed, a valid evaluation DAG order is established once and reused for all simulation vectors.
- If fewer gates are processed than exist in the netlist, a combinational cycle exists. `netpower` halts and lists all gates blocked in the cycle.

### 2. Random-Vector Simulation
- Primary inputs are independently assigned values sampled from Bernoulli(0.5) using `std::mt19937_64`.
- Gates are evaluated in topological order, propagating values to all nets.
- For $N$ vectors, transitions are counted across $N - 1$ vector-to-vector transitions. Startup initialization values do not count as transitions.
- Total transitions $T_{\text{total}} = T_{\text{rising}} + T_{\text{falling}}$.

### 3. Dynamic Power Model
$$P_{\text{dynamic}} = 0.5 \times \alpha \times C \times V^2 \times f$$

Where:
- $\alpha_{\text{net}} = \frac{T_{\text{total, net}}}{N - 1}$ is the total switching activity per interval.
- $C = C_{\text{fF}} \times 10^{-15} \text{ F}$.
- $V$ is supply voltage in Volts.
- $f$ is operating frequency in Hertz.
- Calculated in Watts and reported in nanowatts ($P_{\text{nW}} = P_{\text{W}} \times 10^9$).
- Total dynamic power is the sum of dynamic power over all modeled nets.

---

## Generated Reports

### `summary.txt` Excerpt
```text
================================================================================
Netpower Dynamic Power Estimation Summary
================================================================================
Design Information:
  Module Name:                 half_adder
  Total Nets:                  4
  Primary Inputs:              2
  Primary Outputs:             2
  Logic Gates:                 2

Simulation Parameters:
  Vectors:                     10000
  Measured Intervals:          9999
  Random Seed:                 42

Electrical Parameters:
  Operating Voltage:           1.000000 V
  Operating Frequency:         100000000.000000 Hz
  Default Capacitance:         2.0000 fF
  Capacitance Overrides:       0

Estimated Power Results:
  Total Dynamic Power:         188.098810 nW
  Total Dynamic Power (Watts): 1.880988e-07 W
...
```

### `net_activity.csv`
```csv
net,role,transitions,rising_transitions,falling_transitions,alpha,capacitance_fF,power_nW
a,input,4996,2498,2498,0.499650,2.0000,49.964996
b,input,5057,2529,2528,0.505751,2.0000,50.575058
carry,output,3762,1881,1881,0.376238,2.0000,37.623762
sum,output,4993,2497,2496,0.499350,2.0000,49.934993
```

---

## Testing & Automation

### One-Command CI Script
```bash
bash scripts/ci.sh --build-dir build --build-type Debug
bash scripts/ci.sh --build-dir build-release --build-type Release
```

### CTest Test Suite
```bash
ctest --test-dir build --output-on-failure
```

### Golden File Regression
```bash
bash scripts/run_regression.sh
```

To update golden files deliberately after verified changes:
```bash
bash scripts/run_regression.sh --update-goldens
```

### Sanitizer Build (ASan + UBSan)
```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DNETPOWER_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

---

## Documentation Links

- [System Architecture](docs/ARCHITECTURE.md)
- [Netlist Format Specification](docs/NETLIST_FORMAT.md)
- [Power Model & Formulations](docs/POWER_MODEL.md)
- [Development & Release Workflow](docs/DEVELOPMENT_WORKFLOW.md)
- [Implementation Plan](docs/IMPLEMENTATION_PLAN.md)

