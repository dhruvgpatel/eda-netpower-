# Antigravity Build Brief: Gate-Level Netlist Parser & Dynamic Power Estimator

> **Purpose:** This file is both the project specification and the complete prompt to give Google Antigravity. Open the folder containing this file as the workspace, then paste the contents of the **Build prompt** section into the Antigravity Agent.

## 1. Build prompt — paste into Antigravity

```text
ROLE
You are a senior C++17 / EDA software engineer. Build a complete, maintainable, tested command-line EDA-style application named `netpower`: a Gate-Level Netlist Parser and Dynamic Power Estimator. Work directly in the currently opened repository. Do not merely explain what should be done or produce pseudocode. Create the files, compile the application, run all tests, fix failures, and leave the repository in a usable state.

PROJECT OUTCOME
The tool must:
1. Parse a clearly documented, intentionally limited subset of structural Verilog gate-level netlists.
2. Build an in-memory representation of nets, primary inputs/outputs, and logic gates.
3. Validate connectivity and construct a gate-dependency graph.
4. Detect combinational loops and produce a deterministic topological evaluation order with Kahn's algorithm.
5. Simulate reproducible random input vectors and evaluate the combinational circuit.
6. Calculate per-net transition counts, switching activity, and dynamic power.
7. Generate deterministic text and CSV reports.
8. Include unit tests, golden-file regression tests, Bash automation, CMake/CTest, and GitHub Actions CI/CD.

IMPORTANT WORKING RULES
- First inspect the existing workspace. Preserve useful existing files; do not overwrite unrelated work.
- Before implementation, create a concise implementation plan with milestones and expected files. Then continue into implementation; do not stop after presenting the plan.
- Implement in small phases. After each phase, compile or run the relevant tests before moving on.
- Do not ask me to choose routine engineering details. Use the decisions in this specification.
- Do not claim a test passed unless you actually ran it and observed a successful exit code.
- If a command fails, inspect the failure, fix the underlying issue, and rerun the affected test.
- Do not silently weaken, skip, or delete tests to obtain a green build.
- Never fabricate EDA accuracy. Clearly label this as a zero-delay, vector-based educational estimator, not a sign-off power-analysis tool.
- Use C++17 and the C++ standard library only. Do not add a third-party parser, test framework, or runtime dependency.
- Use clean interfaces, const-correctness, RAII, explicit error handling, and meaningful comments. Avoid global mutable state.
- Keep generated build files and local reports out of Git.

TECHNICAL BASELINE
- Language: C++17.
- Build: CMake (minimum 3.20) and CTest.
- Compiler: GCC and Clang on Linux.
- CLI executable: `netpower`.
- Tests: a lightweight dependency-free C++ test harness registered with CTest.
- Automation: Bash scripts under `scripts/`.
- CI/CD: GitHub Actions under `.github/workflows/`.
- No network access or external libraries are required to build or run the program.

SUPPORTED NETLIST INPUT — VERSION 1
Implement a lexer and parser for a documented scalar, single-module structural Verilog subset. Do not attempt to parse all of IEEE Verilog.

Supported constructs:
- One module per file, with a non-ANSI port list, for example `module half_adder(a, b, sum, carry);`.
- Scalar declarations: `input a, b;`, `output sum, carry;`, and `wire n1, n2;`.
- One primitive gate instance per statement, with positional connections, for example `xor G1 (sum, a, b);`.
- Gate primitives: `and`, `or`, `not`, `nand`, `nor`, `xor`, `xnor`, and `buf`.
- One output terminal per primitive instance.
- `and`, `or`, `nand`, `nor`, `xor`, and `xnor` accept two or more inputs. `not` and `buf` accept exactly one input.
- Scalar constant inputs `1'b0` and `1'b1`.
- Whitespace, newlines, `//` comments, and `/* ... */` comments.
- Gate and net identifiers beginning with a letter or underscore and continuing with letters, digits, `_`, or `$`.

Example supported input:

    module half_adder(a, b, sum, carry);
      input a, b;
      output sum, carry;
      wire n1;
      xor G1 (sum, a, b);
      and G2 (carry, a, b);
    endmodule

Parser/validation rules:
- Track source line and column and report actionable errors such as `file.v:12:8: duplicate driver for net 'n1'`.
- Check module/header ports against declarations.
- Reject duplicate declarations, undeclared signal references, multiple gate drivers, invalid gate arity, missing semicolons, malformed constants, unsupported constructs, and malformed module structure.
- Primary inputs are sources and must not also be driven by a gate.
- A primary output may be driven by a gate or may directly alias a primary input.
- Gate output nets must be declared as `wire` or `output`.
- No implicit nets. No vector buses, parameters, `assign`, `always`, `reg`, `inout`, hierarchy, delays, specify blocks, UDPs, sequential cells, named port connections, or multiple modules. Reject unsupported constructs clearly; do not misparse them.
- Do not rely on line-based parsing or broad regular expressions. Tokenize first, then parse tokens.

GATE LOGIC SEMANTICS
- Implement correct Boolean semantics for every supported primitive.
- `xor` is odd parity over all its inputs; `xnor` is its complement.
- Constants are evaluated as literal Boolean values and do not create graph nodes.
- Add truth-table tests for all gate types, including multi-input XOR/XNOR and constant inputs.

INTERNAL DATA MODEL AND GRAPH
- Create distinct types or clearly separated structures for `Net`, `Gate`, and `Circuit`.
- Each net must retain its name, declaration/role, optional driver gate, fanout gate indices, and simulation value/activity counters.
- Each gate must retain its instance name, primitive type, output net, and ordered input references (which may be net references or constants).
- Validate that every non-constant gate input is declared and has a legal source (primary input or exactly one gate driver).
- Build a gate-dependency directed graph: an edge A -> B exists when a net driven by gate A is consumed by gate B.
- Deduplicate graph edges when the same driven net appears more than once as an input to one consumer gate. Keep indegree accounting consistent with this deduplication.
- Use Kahn's topological sort. Make its output deterministic (for example, use a min-priority queue keyed by original gate index).
- If fewer gates are sorted than the circuit contains, report a combinational cycle. Include names of gates still blocked by the cycle in the diagnostic where practical.
- Compute the topological order once after parsing/validation, not once per random vector. Reuse it for every vector.
- Handle empty gate graphs and direct primary-input-to-primary-output aliases safely.

RANDOM-VECTOR SIMULATION
- Use `std::mt19937_64` and an explicitly supplied seed. The same netlist, options, and seed must produce identical reports across repeated runs on the same supported platform/toolchain.
- Generate each primary input independently with a Bernoulli(0.5) distribution for each vector.
- For each vector: assign primary inputs, then evaluate all gates exactly once in the precomputed topological order.
- Compare each completed vector with the previous completed vector to update net transitions. Do not count an undefined startup value as a transition.
- A run with N vectors has N-1 measured vector-to-vector intervals. Require at least two vectors.
- Count total transitions (both 0->1 and 1->0) for every net. Also retain rising and falling counts for reporting/debugging.
- This is zero-delay Boolean simulation. Do not claim to model glitches, hazards, inertial/transport delay, sequential timing, clock trees, or analog effects.

DYNAMIC POWER MODEL AND DEFINITIONS
Use the requested model:

    P_dynamic = 0.5 * alpha * C * V^2 * f

Define `alpha` explicitly as the **total number of 0<->1 transitions per net per measured vector interval**:

    alpha_net = total_transitions_net / (number_of_vectors - 1)

With this definition, the factor 0.5 represents average energy per transition. Do not redefine alpha as only the 0->1 probability while retaining the 0.5 factor. Explain this convention in README and report headers.

Units and calculations:
- CLI voltage is in volts (V).
- CLI frequency is in hertz (Hz); do not interpret MHz silently.
- Capacitance input is in femtofarads (fF), converted internally to farads using `C_F = C_fF * 1e-15`.
- Power is computed in watts and displayed in nanowatts using `P_nW = P_W * 1e9`.
- Per-net power: `0.5 * alpha_net * C_net_F * V_V * V_V * f_Hz`.
- Total dynamic power is the sum of per-net estimates for all modeled nets.
- A net with zero transitions has zero estimated dynamic power.
- Support a default capacitance applied to every net and an optional CSV override file with header `net,capacitance_fF`. An explicit per-net value overrides the default.
- Reject negative, non-finite, or malformed voltage/frequency/capacitance values. Require voltage > 0, frequency > 0, capacitance >= 0, vectors >= 2.
- Reject duplicate capacitance rows and capacitance entries for unknown nets.
- Clearly state that this simple model estimates switched net capacitance only. It excludes leakage, internal cell power, short-circuit power, timing/glitch effects, clock-tree power, state-dependent cell power, and technology-library characterization.

COMMAND-LINE INTERFACE
Implement robust argument parsing and `--help`. Required options:

    netpower \
      --netlist examples/half_adder.v \
      --vectors 10000 \
      --seed 42 \
      --voltage 1.0 \
      --frequency-hz 100000000 \
      --default-capacitance-ff 2.0 \
      --output-dir results/run-001

Optional:
- `--capacitance-file path.csv`
- `--help`

Requirements:
- Missing options, unknown flags, invalid numbers, unreadable files, and invalid output paths must produce a concise diagnostic and non-zero exit status.
- Print a useful summary to stdout, while writing stable report files into the output directory.
- Do not put timestamps, absolute workspace paths, or other nondeterministic values in golden-comparison reports.
- Keep numeric precision and row ordering stable. Sort per-net CSV rows by net name.

OUTPUT FILES
Generate:
1. `summary.txt` — module name, gate/net/input/output counts, vector count, measured interval count, seed, voltage, frequency, capacitance convention, total estimated dynamic power and model limitations.
2. `net_activity.csv` — header exactly:

       net,role,transitions,rising_transitions,falling_transitions,alpha,capacitance_fF,power_nW

   Include every modeled net exactly once. Use stable role labels such as `input`, `internal`, `output`, or `input_output`.
3. Optionally `run_config.txt` if useful, but keep it deterministic and do not add timestamps.

REPOSITORY STRUCTURE
Create a clean structure close to this (adjust only when there is a clear engineering reason):

    .
    ├── .github/
    │   └── workflows/
    │       ├── ci.yml
    │       └── release.yml
    ├── .agents/
    │   └── rules/
    │       └── cpp-project.md          # optional if useful; root AGENTS.md is mandatory
    ├── docs/
    │   ├── ARCHITECTURE.md
    │   ├── NETLIST_FORMAT.md
    │   ├── POWER_MODEL.md
    │   └── DEVELOPMENT_WORKFLOW.md
    ├── examples/
    │   ├── half_adder.v
    │   ├── full_adder.v
    │   └── inverter.v
    ├── include/netpower/
    │   ├── circuit.hpp
    │   ├── parser.hpp
    │   ├── graph.hpp
    │   ├── simulator.hpp
    │   ├── power.hpp
    │   ├── report.hpp
    │   └── cli.hpp
    ├── src/
    │   ├── main.cpp
    │   ├── parser.cpp
    │   ├── graph.cpp
    │   ├── simulator.cpp
    │   ├── power.cpp
    │   ├── report.cpp
    │   └── cli.cpp
    ├── tests/
    │   ├── unit/
    │   │   └── test_main.cpp (or small grouped test files)
    │   ├── fixtures/
    │   │   ├── half_adder.v
    │   │   ├── combinational_loop.v
    │   │   ├── multiple_driver.v
    │   │   └── malformed.v
    │   └── golden/
    │       ├── half_adder_summary.txt
    │       └── half_adder_net_activity.csv
    ├── scripts/
    │   ├── ci.sh
    │   └── run_regression.sh
    ├── .gitignore
    ├── .clang-format
    ├── AGENTS.md
    ├── CMakeLists.txt
    ├── LICENSE
    └── README.md

Use a sensible, cohesive API. The exact number of source/header files may change, but do not put the entire application in `main.cpp`.

TESTING REQUIREMENTS
Create dependency-free unit tests and register them with CTest. At minimum test:
- Truth tables for AND, OR, NOT, NAND, NOR, XOR, XNOR, and BUF.
- Multi-input XOR/XNOR behavior.
- Constant 0/1 input handling.
- Valid parsing of the example circuits and whitespace/comments/multiline statements.
- Parser errors: undeclared net, duplicate declaration, malformed syntax, invalid arity, unsupported construct, multiple module definitions, and malformed constant.
- Multiple drivers and primary-input-driven-net errors.
- Correct topological ordering and deterministic tie handling.
- Combinational-loop detection, including a self-loop if representable by the supported grammar.
- Direct primary-input-to-primary-output alias.
- Reproducibility with a fixed random seed.
- Correct transition counts and alpha denominator (N-1).
- Power scaling: doubling V multiplies power by 4; doubling f multiplies power by 2; doubling C multiplies power by 2; zero transitions gives zero power.
- Invalid CLI values and missing files.

GOLDEN-FILE REGRESSION
- Build a deterministic end-to-end regression using `examples/half_adder.v`, fixed seed, fixed vector count, and fixed electrical parameters.
- `scripts/run_regression.sh` must invoke the built CLI, write results to a temporary directory, and compare `summary.txt` and `net_activity.csv` with checked-in golden files using `diff -u` (or equivalent).
- The script must fail on any mismatch, print readable diagnostics, clean temporary files with a trap, and work from any current working directory by resolving the repository root from the script location.
- The script must not rewrite golden files automatically. Provide a separate documented, deliberate command for updating goldens after reviewing the diff.
- Register the regression script as a CTest integration test.

BASH CI SCRIPT
Create `scripts/ci.sh` with strict shell behavior (`set -euo pipefail`). It must:
1. Resolve the repository root regardless of the caller's current directory.
2. Accept optional build directory and build type (Debug by default).
3. Configure with CMake and `BUILD_TESTING=ON`.
4. Build the project.
5. Run CTest with `--output-on-failure`.
6. Return non-zero on any configure/build/test failure.

CMAKE / CODE QUALITY
- Require CMake 3.20 or newer.
- Set C++ standard to 17, required, extensions off.
- Build the `netpower` executable and test executable(s).
- Enable CTest and register unit and regression tests.
- Enable useful warnings (`-Wall -Wextra -Wpedantic`) for GCC/Clang without compiler-specific breakage.
- Provide an optional sanitizer build option for AddressSanitizer and UndefinedBehaviorSanitizer on supported GCC/Clang configurations.
- Use a consistent output path such as `${CMAKE_BINARY_DIR}/bin/netpower` so scripts and CI have a stable executable path.
- Do not make warnings-as-errors the only way to build on all compilers.
- Add `.gitignore` entries for build directories, generated results, object files, and IDE clutter.
- Add `.clang-format` with a conventional readable C++ style.

GITHUB ACTIONS — CONTINUOUS INTEGRATION
Create `.github/workflows/ci.yml`:
- Trigger on pull requests targeting `main`, pushes to `main`, and `workflow_dispatch`.
- Use `ubuntu-latest`.
- Use a matrix covering GCC and Clang, and Debug and Release builds.
- Install CMake, Ninja/build tools, GCC, and Clang.
- Run the same `scripts/ci.sh` used locally.
- Set minimal repository permissions (`contents: read`), a job timeout, and concurrency cancellation for superseded runs.
- Ensure all CTest tests, including golden-file regression, run in CI.
- No secrets are needed for CI.

GITHUB ACTIONS — CONTINUOUS DELIVERY
Create `.github/workflows/release.yml`:
- Trigger on version tags matching `v*.*.*`.
- Build a Release Linux binary and run the full test suite before packaging.
- Stage the CLI executable, README, LICENSE, and example netlists into a release directory.
- Create a `.tar.gz` archive and a SHA-256 checksum file.
- Publish the files to a GitHub Release using the GitHub CLI (`gh`) and the repository's `GITHUB_TOKEN`; set only the needed `contents: write` permission.
- Do not deploy to a server: this is a command-line application, so CD means creating a tested, downloadable release artifact.
- Include a manual release procedure in `docs/DEVELOPMENT_WORKFLOW.md`: update version/changelog, run local CI, commit, create and push a semantic version tag such as `v1.0.0`, then inspect the GitHub Actions and Release pages.

DOCUMENTATION
Write a useful README with:
- Project overview and feature list.
- Supported netlist grammar and a small example.
- Dependencies and build instructions for Ubuntu.
- Exact run commands and CLI option descriptions.
- Explanation of Kahn's algorithm and cycle detection.
- Simulation model and deterministic seed behavior.
- The exact alpha convention and units in the power equation.
- Limitations and non-sign-off disclaimer.
- Unit, regression, sanitizer, CI, and release commands.
- Example report excerpts.

Also write `docs/ARCHITECTURE.md`, `docs/NETLIST_FORMAT.md`, `docs/POWER_MODEL.md`, and `docs/DEVELOPMENT_WORKFLOW.md` with enough detail for another engineer to maintain the project.

IMPLEMENTATION MILESTONES
Milestone 1 — Repository/build skeleton:
- Create root AGENTS.md, CMake project, folder structure, example netlists, basic CLI skeleton, and README outline.
- Configure and compile a minimal executable.

Milestone 2 — Lexer/parser and data model:
- Implement tokenization with source locations, parser, declarations, gates, constants, and semantic validation.
- Add parser and primitive truth-table tests.

Milestone 3 — Graph and validation:
- Build gate dependencies, deterministic Kahn sort, and cycle diagnostics.
- Add topological-order, duplicate-driver, undriven-net, and loop tests.

Milestone 4 — Simulation:
- Implement deterministic random vectors, topological evaluation, and transition accounting.
- Add reproducibility and activity tests.

Milestone 5 — Power and reports:
- Implement capacitance CSV overrides, unit conversions, power calculations, and deterministic reports.
- Add scaling and CLI validation tests.

Milestone 6 — Regression and automation:
- Implement golden output files, run_regression.sh, ci.sh, CTest registration, and local validation.

Milestone 7 — CI/CD and documentation:
- Create GitHub Actions workflows, finish documentation, run full Debug and Release builds, and test the release packaging commands locally where possible.

ACCEPTANCE CHECKLIST — DO NOT MARK COMPLETE UNTIL VERIFIED
- [ ] Clean CMake configure succeeds.
- [ ] Debug build succeeds with GCC.
- [ ] Release build succeeds with GCC.
- [ ] Build/test succeeds with Clang where available.
- [ ] CTest unit tests pass.
- [ ] Golden regression test passes.
- [ ] Combinational loops are rejected with a useful message.
- [ ] Same seed produces identical reports.
- [ ] Voltage, frequency, and capacitance scaling tests pass.
- [ ] `netpower --help` works.
- [ ] Example half-adder run produces summary.txt and net_activity.csv.
- [ ] `scripts/ci.sh` succeeds from both repository root and another working directory.
- [ ] CI and release workflows are syntactically valid and consistent with the actual project commands/paths.
- [ ] README and design documents accurately describe the implemented behavior.
- [ ] Git status contains no generated build outputs or temporary reports.

FINAL RESPONSE WHEN FINISHED
Provide:
1. A concise architecture summary.
2. The final repository tree.
3. Exact build, test, example-run, sanitizer, and release commands.
4. The actual test commands run and their observed results.
5. Any limitations or incomplete items; never hide them.
6. A short explanation of how to push the repository to GitHub and trigger CI/CD.

Start by inspecting the workspace, then create the implementation plan and execute all milestones. Do not stop at the plan.
```

## 2. Repository setup in Antigravity

1. Create a new folder, for example `gate-level-netpower`.
2. Extract this starter pack into that folder, preserving `.github/workflows/` and `AGENTS.md`.
3. Open the folder as the Antigravity workspace.
4. Open `ANTIGRAVITY_BUILD_PROMPT.md`, copy the complete text inside **Build prompt — paste into Antigravity**, and paste it into the Agent panel.
5. Ask the Agent to implement the milestones in order, run the checks, and show you the actual terminal results before it reports completion.
6. Review the implementation plan and diffs. Do not approve broad destructive changes or commits you have not inspected.

The root `AGENTS.md` gives Antigravity persistent project constraints. Keep it in the repository so the same instructions travel with the code.

## 3. Development workflow

| Stage | Work | Required evidence before proceeding |
|---|---|---|
| 1. Scaffold | CMake, folders, CLI skeleton, examples | Clean configure and minimal build |
| 2. Parse | Lexer, parser, semantic validation | Parser tests and gate truth tables pass |
| 3. Graph | Driver/fanout graph, Kahn sort, loop detection | DAG order tests and cycle tests pass |
| 4. Simulate | Seeded vectors, gate evaluation, transitions | Deterministic replay and activity tests pass |
| 5. Estimate | Capacitance mapping, power, reports | V², f, C scaling tests pass |
| 6. Regression | Golden files and Bash scripts | `ctest --output-on-failure` passes |
| 7. CI | GCC/Clang × Debug/Release | GitHub Actions matrix is green |
| 8. CD | Tag-triggered archive and checksum | GitHub Release contains tested artifacts |

### Local commands (expected interface)

```bash
# Ubuntu dependencies
sudo apt update
sudo apt install -y build-essential cmake ninja-build clang git

# Configure/build/test using the project's one-command script
bash scripts/ci.sh --build-dir build --build-type Debug
bash scripts/ci.sh --build-dir build-release --build-type Release

# Run an example after a successful build
./build/bin/netpower \
  --netlist examples/half_adder.v \
  --vectors 10000 \
  --seed 42 \
  --voltage 1.0 \
  --frequency-hz 100000000 \
  --default-capacitance-ff 2.0 \
  --output-dir results/half-adder

# Inspect reports
cat results/half-adder/summary.txt
column -s, -t < results/half-adder/net_activity.csv | less -S

# Optional sanitizer build (the project should document its exact option)
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DNETPOWER_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

The exact sanitizer option name above is part of the requested interface; Antigravity must implement it or document a clearly equivalent option and make all scripts/docs consistent.

## 4. CI/CD lifecycle

### Continuous Integration (every pull request and push to `main`)

```text
Pull request / push to main
          |
          v
   GitHub Actions runner
          |
          +--> GCC + Debug ---- configure -> build -> CTest -> golden diff
          |
          +--> GCC + Release -- configure -> build -> CTest -> golden diff
          |
          +--> Clang + Debug -- configure -> build -> CTest -> golden diff
          |
          +--> Clang + Release configure -> build -> CTest -> golden diff
          |
          v
    Green checks required before merge
```

### Continuous Delivery (only when a version tag is pushed)

```text
Create annotated tag (example: v1.0.0)
          |
          v
Push tag to GitHub
          |
          v
Release workflow: configure Release -> build -> run full CTest
          |
          v
Stage binary + README + LICENSE + examples
          |
          v
Create .tar.gz + SHA-256 checksum
          |
          v
Publish GitHub Release with downloadable assets
```

Suggested Git commands after reviewing and committing the implementation:


## 5. Engineering notes and scope boundaries

- This is a portfolio-quality educational EDA tool, not a replacement for commercial sign-off tools such as PrimeTime PX or gate-level power flows driven by characterized Liberty data.
- Random independent 0.5-probability input vectors are useful for deterministic demonstrations but are not workload traces. Realistic activity depends on input distributions and temporal correlation.
- A single capacitance default is a simplifying assumption. Per-net CSV overrides make experiments more informative, but the tool does not infer wire capacitance or load from placement/routing.
- A zero-delay topological simulator does not reproduce hazards or glitches. Those can materially affect switching power in real circuits.
- Keep these limitations in the README and generated summary so the project is technically honest.

## 6. Antigravity customization reference

The included root `AGENTS.md` is the persistent instruction file. Antigravity's current documentation describes workspace-level `AGENTS.md`/`GEMINI.md` and `.agents/rules/` files, and separately documents Agent Skills for reusable multi-step procedures. Prefer the root `AGENTS.md` for always-on project constraints; do not depend on legacy saved-workflow behavior for this build.
