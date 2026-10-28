#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

UPDATE_GOLDENS=false
for arg in "$@"; do
    if [[ "$arg" == "--update-goldens" ]]; then
        UPDATE_GOLDENS=true
    fi
done

# Locate netpower binary
if [[ -n "${NETPOWER_BIN:-}" && -x "${NETPOWER_BIN}" ]]; then
    EXE="${NETPOWER_BIN}"
elif [[ -x "${REPO_ROOT}/build/bin/netpower" ]]; then
    EXE="${REPO_ROOT}/build/bin/netpower"
elif [[ -x "${REPO_ROOT}/build-release/bin/netpower" ]]; then
    EXE="${REPO_ROOT}/build-release/bin/netpower"
else
    # Fallback search in build directories
    EXE="$(find "${REPO_ROOT}" -type f -name netpower -perm +111 2>/dev/null | head -n 1 || true)"
    if [[ -z "${EXE}" || ! -x "${EXE}" ]]; then
        echo "Error: netpower executable not found. Please build the project first." >&2
        exit 1
    fi
fi

GOLDEN_DIR="${REPO_ROOT}/tests/golden"
TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/netpower_reg_XXXXXX")"
trap 'rm -rf "${TMP_DIR}"' EXIT

echo "Running golden-file regression test using: ${EXE}"

"${EXE}" \
  --netlist "${REPO_ROOT}/examples/half_adder.v" \
  --vectors 10000 \
  --seed 42 \
  --voltage 1.0 \
  --frequency-hz 100000000 \
  --default-capacitance-ff 2.0 \
  --output-dir "${TMP_DIR}" > /dev/null

if [[ "${UPDATE_GOLDENS}" == true ]]; then
    echo "Updating golden files in ${GOLDEN_DIR}..."
    cp "${TMP_DIR}/summary.txt" "${GOLDEN_DIR}/half_adder_summary.txt"
    cp "${TMP_DIR}/net_activity.csv" "${GOLDEN_DIR}/half_adder_net_activity.csv"
    echo "Golden files updated successfully."
    exit 0
fi

# Compare summary.txt
if ! diff -u "${GOLDEN_DIR}/half_adder_summary.txt" "${TMP_DIR}/summary.txt"; then
    echo "ERROR: Golden regression mismatch in summary.txt!" >&2
    exit 1
fi

# Compare net_activity.csv
if ! diff -u "${GOLDEN_DIR}/half_adder_net_activity.csv" "${TMP_DIR}/net_activity.csv"; then
    echo "ERROR: Golden regression mismatch in net_activity.csv!" >&2
    exit 1
fi

echo "Golden regression test PASSED: all outputs match golden files exactly."
exit 0

