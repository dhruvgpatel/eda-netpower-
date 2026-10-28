# Project Instructions for Antigravity

You are working on `netpower`, a C++17 gate-level netlist parser and dynamic power estimator.

## Non-negotiable constraints
- Follow `ANTIGRAVITY_BUILD_PROMPT.md` as the source of truth for requirements.
- Implement real code, compile it, and run tests; do not stop at plans or pseudocode.
- Use C++17, CMake, CTest, Bash, and the C++ standard library only unless the user explicitly changes scope.
- Keep the supported structural Verilog subset explicit. Reject unsupported syntax with file/line/column diagnostics; never pretend to parse full Verilog.
- Build the gate dependency order once per netlist using deterministic Kahn topological sorting and reuse it across vectors.
- Make random simulation deterministic for a fixed seed.
- Define alpha as total 0-to-1 plus 1-to-0 transitions per measured vector interval. Use `P = 0.5 * alpha * C * V^2 * f` with documented units.
- Keep reports deterministic: no timestamps or absolute paths in golden-comparison files.
- Add tests for parser errors, truth tables, loops, reproducibility, transition accounting, and power scaling.
- Never delete or weaken tests to make CI pass.
- Do not claim verification without running the corresponding command.
- Do not commit generated build directories, temporary reports, or local editor state.

## Engineering style
- Prefer small cohesive modules, clear interfaces, const-correctness, RAII, and descriptive errors.
- Compile with `-Wall -Wextra -Wpedantic` on GCC/Clang.
- Keep `main.cpp` limited to application orchestration and exit-code handling.
- Document assumptions and limitations, especially zero-delay simulation and the absence of sign-off accuracy.
- Run `scripts/ci.sh` after meaningful changes and before declaring completion.
