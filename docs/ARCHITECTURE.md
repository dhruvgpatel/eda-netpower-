# netpower: System Architecture

`netpower` is designed with small, cohesive, decoupled modules with clear interfaces, RAII resource management, strict const-correctness, and explicit error handling without global mutable state.

## System Block Diagram

```text
       +-----------------------+
       | Structural Verilog   |
       | (.v input file)       |
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       |   Lexer & Parser      | <--- Syntax / Semantic Validation
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       |   Circuit IR Model    | (Nets, Gates, Ports)
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       | Dependency Graph &    | <--- Kahn's Topological Sort
       | Cycle Detection       | <--- Edge Deduplication
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       | Vector Simulator      | <--- PRNG (std::mt19937_64)
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       | Dynamic Power Engine  | <--- P = 0.5 * alpha * C * V^2 * f
       +-----------+-----------+
                   |
                   v
       +-----------------------+
       | Report Generator      | ---> summary.txt
       |                       | ---> net_activity.csv
       +-----------------------+
```

## Module Descriptions

### 1. `circuit` (`include/netpower/circuit.hpp`, `src/circuit.cpp`)
- Data structures: `Net`, `Gate`, `Circuit`.
- Enumerations: `GateType` (`AND`, `OR`, `NOT`, `NAND`, `NOR`, `XOR`, `XNOR`, `BUF`), `NetRole` (`INPUT`, `INTERNAL`, `OUTPUT`, `INPUT_OUTPUT`), `InputKind` (`NET`, `CONST_0`, `CONST_1`).
- Boolean logic evaluation function `evaluate_gate(GateType, const std::vector<bool>&)` implementing standard truth table semantics and multi-input odd parity for XOR / even parity for XNOR.

### 2. `parser` (`include/netpower/parser.hpp`, `src/parser.cpp`)
- Lexer: Scans tokens from input stream with precise line and column tracking. Supports single-line (`//`) and multi-line (`/* */`) comments.
- Parser: Parses module header, scalar declarations (`input`, `output`, `wire`), and gate instantiations.
- Validation:
  - Enforces declaration of all header ports.
  - Rejects undeclared nets, duplicate declarations, and multiple drivers.
  - Rejects primary inputs driven by gates.
  - Rejects gate arity violations.
  - Rejects unsupported constructs (`assign`, `always`, `reg`, vectors, etc.).

### 3. `graph` (`include/netpower/graph.hpp`, `src/graph.cpp`)
- Builds gate dependency DAG ($G_A \to G_B$ when gate $G_A$ drives an input consumed by $G_B$).
- Deduplicates edges between gates to preserve accurate in-degree accounting.
- Implements deterministic Kahn's topological sort using `std::priority_queue` keyed by original gate index.
- Detects combinational loops and reports diagnostic listing all gates blocked by the cycle.

### 4. `simulator` (`include/netpower/simulator.hpp`, `src/simulator.cpp`)
- Seeded random vector generator using `std::mt19937_64` and `std::bernoulli_distribution(0.5)`.
- Reuses precomputed topological gate order for all vectors.
- Tracks rising ($0 \to 1$), falling ($1 \to 0$), and total transitions.
- Computes switching activity $\alpha_{\text{net}} = \frac{T_{\text{total}}}{N - 1}$.

### 5. `power` (`include/netpower/power.hpp`, `src/power.cpp`)
- Parses per-net capacitance CSV overrides (`net,capacitance_fF`).
- Computes per-net and total circuit dynamic power:
  $$P = 0.5 \times \alpha \times C \times V^2 \times f$$
- Validates electrical parameters ($V > 0$, $f > 0$, $C \ge 0$).

### 6. `report` (`include/netpower/report.hpp`, `src/report.cpp`)
- Produces deterministic, reproducible `summary.txt` and `net_activity.csv`.
- Sorts CSV rows alphabetically by net name.
- Omits nondeterministic timestamps and absolute paths to ensure golden comparison stability.

### 7. `cli` (`include/netpower/cli.hpp`, `src/cli.cpp`)
- Robust command-line option parser for required and optional flags.
- Comprehensive input range checks and informative `--help`.

### 8. `main` (`src/main.cpp`)
- Orchestration and top-level exception handling with clean exit codes.

