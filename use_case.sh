#!/bin/bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INPUT_DIR="$ROOT_DIR/input_data"
TARGET_FILE="$INPUT_DIR/input.json"

get_cases() {
    find "$INPUT_DIR" -maxdepth 1 -type f -name 'case_*.json' -printf '%f\n' | sort
}

list_cases() {
    local i=1
    while IFS= read -r case_file; do
        echo "$i) $case_file"
        i=$((i + 1))
    done < <(get_cases)
}

run_selected_case() {
    local selected_case="$1"
    local source_file="$INPUT_DIR/$selected_case"

    if [[ ! -f "$source_file" ]]; then
        echo "[ERROR] Case file not found: $selected_case"
        exit 1
    fi

    cp "$source_file" "$TARGET_FILE"
    echo "[OK] Activated case: $selected_case"
    echo "[OK] Updated: input_data/input.json"
    echo "[INFO] Starting run.sh..."

    bash "$ROOT_DIR/run.sh"
}

if [[ ${1:-} == "--list" || ${1:-} == "-l" ]]; then
    echo "Available test case files:"
    list_cases
    exit 0
fi

if [[ $# -eq 1 ]]; then
    # Backward compatibility: allow direct file name usage.
    run_selected_case "$1"
    exit 0
fi

# Interactive mode (default): show numbered list and ask for serial number.
mapfile -t CASES < <(get_cases)

if [[ ${#CASES[@]} -eq 0 ]]; then
    echo "[ERROR] No case_*.json files found in input_data/"
    exit 1
fi

echo "Select a test case:"
for i in "${!CASES[@]}"; do
    sr_no=$((i + 1))
    echo "$sr_no) ${CASES[$i]}"
done

echo ""
read -r -p "Enter serial number: " selected_no

if ! [[ "$selected_no" =~ ^[0-9]+$ ]]; then
    echo "[ERROR] Please enter a valid number."
    exit 1
fi

if (( selected_no < 1 || selected_no > ${#CASES[@]} )); then
    echo "[ERROR] Serial number out of range."
    exit 1
fi

selected_case="${CASES[$((selected_no - 1))]}"
run_selected_case "$selected_case"
