# netpower: Development & Release Workflow

This document details the software development lifecycle, testing strategies, CI/CD pipelines, and release procedures for `netpower`.

---

## 1. Local Development Setup

### System Requirements
- OS: Linux (Ubuntu 20.04+) or macOS.
- Compiler: GCC 9+ or Clang 10+ with C++17 support.
- CMake: Minimum version 3.20.
- Build system: Make or Ninja.
- Bash: Version 4.0+.

### Configure & Build
```bash
# Debug build (with assertions and debug symbols)
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build --parallel

# Release build (with optimizations)
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-release --parallel
```

### Run Tests
```bash
# Run all registered CTest tests
ctest --test-dir build --output-on-failure

# Run unit tests directly
./build/bin/test_netpower

# Run golden regression test directly
bash scripts/run_regression.sh
```

### Memory & Undefined Behavior Sanitizers
To compile with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan):
```bash
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug -DNETPOWER_ENABLE_SANITIZERS=ON
cmake --build build-asan --parallel
ctest --test-dir build-asan --output-on-failure
```

---

## 2. Continuous Integration (CI)

The CI pipeline is automated via GitHub Actions in `.github/workflows/ci.yml`.

### Triggers
- Push to `main` branch.
- Pull request targeting `main`.
- Manual trigger via `workflow_dispatch`.

### Matrix Strategy
CI executes across:
- Compilers: `g++` and `clang++`
- Build configurations: `Debug` and `Release`

### Automation Script (`scripts/ci.sh`)
The CI workflow runs the exact same script developers use locally:
```bash
bash scripts/ci.sh --build-dir "build-${CXX}-${BUILD_TYPE}" --build-type "${BUILD_TYPE}"
```
This guarantees strict parity between local development environments and CI runners.

---

## 3. Continuous Delivery & Release Procedure

Releases are published to GitHub Releases via `.github/workflows/release.yml` whenever an annotated semantic version tag (e.g. `v1.0.0`) is pushed.

### Step-by-Step Release Procedure

1. **Verify Local Quality Gates**:
   ```bash
   bash scripts/ci.sh --build-dir build --build-type Debug
   bash scripts/ci.sh --build-dir build-release --build-type Release
   bash scripts/run_regression.sh
   ```

2. **Commit Changes**:
   Ensure all working tree changes are clean and committed:
   ```bash
   git status
   git add .
   git commit -m "Release v1.0.0"
   ```

3. **Create and Push Semantic Version Tag**:
   ```bash
   git tag -a v1.0.0 -m "netpower v1.0.0 release"
   git push origin main --tags
   ```

4. **Automated Pipeline Execution**:
   The `release.yml` workflow:
   - Checks out the tagged commit.
   - Builds a clean `Release` Linux binary.
   - Executes the full CTest suite (unit tests and golden regression).
   - Packages the executable, `README.md`, `LICENSE`, and `examples/` into `dist/netpower-v1.0.0-linux-x86_64.tar.gz`.
   - Computes a SHA-256 checksum file.
   - Publishes the GitHub Release with the tarball and checksum attached.

5. **Verify GitHub Release**:
   Navigate to the repository's GitHub Releases page and verify that the release assets and release notes are published.

