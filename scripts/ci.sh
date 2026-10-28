#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${REPO_ROOT}/build"
BUILD_TYPE="Debug"

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --build-dir)
            if [[ -z "${2:-}" ]]; then
                echo "Error: missing argument for --build-dir" >&2
                exit 1
            fi
            BUILD_DIR="$2"
            shift 2
            ;;
        --build-type)
            if [[ -z "${2:-}" ]]; then
                echo "Error: missing argument for --build-type" >&2
                exit 1
            fi
            BUILD_TYPE="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--build-dir <dir>] [--build-type <Debug|Release>]"
            exit 0
            ;;
        *)
            echo "Error: unknown argument '$1'" >&2
            exit 1
            ;;
    esac
done

# If relative path, make it relative to working directory or repo root
if [[ "${BUILD_DIR}" != /* ]]; then
    BUILD_DIR="${PWD}/${BUILD_DIR}"
fi

echo "============================================================"
echo "netpower CI script"
echo "Repository Root: ${REPO_ROOT}"
echo "Build Directory: ${BUILD_DIR}"
echo "Build Type:      ${BUILD_TYPE}"
echo "============================================================"

# 1. Configure with CMake
echo "==> Configuring CMake..."
cmake -S "${REPO_ROOT}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DBUILD_TESTING=ON

# 2. Build the project
echo "==> Building project..."
cmake --build "${BUILD_DIR}" --parallel

# 3. Run CTest
echo "==> Running CTest test suite..."
ctest --test-dir "${BUILD_DIR}" --output-on-failure

echo "==> CI run completed successfully!"

