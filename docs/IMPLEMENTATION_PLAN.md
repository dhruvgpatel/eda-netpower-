# netpower: Implementation Plan

## Milestone 1: Project Skeleton & Build Setup
- `.gitignore`: ignore build dirs, ctest artifacts, results.
- `.clang-format`: Google/LLVM based clean C++ style.
- `LICENSE`: MIT License.
- `CMakeLists.txt`: C++17, warnings (`-Wall -Wextra -Wpedantic`), sanitize option `NETPOWER_ENABLE_SANITIZERS`, `CMAKE_RUNTIME_OUTPUT_DIRECTORY` to `${CMAKE_BINARY_DIR}/bin`.
- Directory structure: `include/netpower`, `src`, `examples`, `tests/unit`, `tests/fixtures`, `tests/golden`, `scripts`, `docs`.
- Example netlists: `half_adder.v`, `full_adder.v`, `inverter.v`.
- Initial `main.cpp` CLI skeleton and basic configure/build verification.

## Milestone 2: Lexer, Parser & Semantic Validation
- `include/netpower/parser.hpp`, `src/parser.cpp`:
  - Tokenizer with line/col tracking (`Token`, `TokenType`, `SourceLocation`).
  - Strict scalar Verilog subset: non-ANSI module header, scalar input/output/wire declarations, gate instances, constants (`1'b0`, `1'b1`), comments (`//`, `/* */`).
  - Detailed syntax and semantic diagnostics (`file:line:col: message`).
  - Validation: undeclared signals, duplicate declarations, multiple drivers, input driven by gate, invalid gate arity, unsupported keywords.
- Logic primitive truth tables evaluation helper.
- Unit tests for tokenizer, parser errors, and primitive truth tables.

## Milestone 3: Data Model, Dependency Graph & Kahn Topological Sort
- `include/netpower/circuit.hpp`, `include/netpower/graph.hpp`, `src/graph.cpp`:
  - `Net`, `Gate`, `Circuit` data structures.
  - Driver-fanout graph construction, multi-fanin deduplication.
  - Kahn's algorithm with deterministic tie-breaking (min-index priority queue).
  - Combinational loop detection with diagnostic listing blocked gates.
  - Handling direct input-to-output aliases and empty gate circuits.
- Unit tests for graph construction, topological order, tie-breaking, and cycle diagnostics.

## Milestone 4: Random-Vector Simulator
- `include/netpower/simulator.hpp`, `src/simulator.cpp`:
  - `std::mt19937_64` deterministic PRNG.
  - Bernoulli(0.5) primary input generation.
  - Multi-vector evaluation using precomputed topological order.
  - Transition accounting: rising (`0->1`), falling (`1->0`), total transitions across $N-1$ intervals.
- Unit tests for simulation repeatability, transition accounting, and direct aliases.

## Milestone 5: Dynamic Power Estimator & Reporting
- `include/netpower/power.hpp`, `src/power.cpp`, `include/netpower/report.hpp`, `src/report.cpp`, `include/netpower/cli.hpp`, `src/cli.cpp`:
  - Capacitance file parser and validator (CSV with `net,capacitance_fF`).
  - Dynamic power calculation: $P = 0.5 \times \alpha \times C \times V^2 \times f$.
  - CLI argument parsing with full validation and `--help`.
  - Deterministic report generation: `summary.txt` and `net_activity.csv`.
- Unit tests for power scaling ($V^2$, $f$, $C$), zero transition power, CSV parser, CLI parsing.

## Milestone 6: Regression Testing & CI Scripts
- `tests/golden/half_adder_summary.txt`, `tests/golden/half_adder_net_activity.csv`.
- `scripts/run_regression.sh`: diff against goldens, temp directory handling, trap cleanup.
- `scripts/ci.sh`: build and test automation accepting `--build-dir` and `--build-type`.
- CMake registration of CTest targets (unit tests and regression).

## Milestone 7: Documentation & CI/CD Verification
- Complete documentation: `README.md`, `docs/ARCHITECTURE.md`, `docs/NETLIST_FORMAT.md`, `docs/POWER_MODEL.md`, `docs/DEVELOPMENT_WORKFLOW.md`.
- Verify `.github/workflows/ci.yml` and `release.yml`.
- Verify full test suite passes locally with `scripts/ci.sh`.

