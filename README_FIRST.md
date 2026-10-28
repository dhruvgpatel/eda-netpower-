# Start here — Antigravity project starter

1. Open this extracted folder as your Antigravity workspace.
2. Open `ANTIGRAVITY_BUILD_PROMPT.md`.
3. Copy the entire text in section **1. Build prompt — paste into Antigravity** (the text inside the fenced `text` block) into the Antigravity Agent.
4. Let the Agent create and test the implementation in milestones. Review its plan and diffs as it works.
5. Keep `AGENTS.md`, `.github/workflows/ci.yml`, and `.github/workflows/release.yml` in the repository. The prompt asks Antigravity to reconcile and test these workflows against the actual scripts and CMake targets it creates.

## Important

The CI and release YAML files are starter templates. Antigravity must verify them against the final CMake target paths, script names, and project files before the first push. The release workflow expects the executable at `build-release/bin/netpower` and a root `LICENSE` file.