#!/bin/bash

# -----------------------------------------------------------------------------
# Run the full stack locally:
# 1) Compile and run backend engine (C++).
# 2) Start frontend dev server (Vite).
# 3) Open http://localhost:3000 in the default browser.
# -----------------------------------------------------------------------------

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BACKEND_SRC="$ROOT_DIR/Backend/src"
FRONTEND_DIR="$ROOT_DIR/Frontend"
SUGGESTIONS_DIR="$ROOT_DIR/Backend/suggestions"
TESTDATA_DIR="$ROOT_DIR/Backend/testdata"

echo ">> Cleaning old generated files..."
if [ -d "$SUGGESTIONS_DIR" ]; then
    find "$SUGGESTIONS_DIR" -type f -delete
fi
if [ -d "$TESTDATA_DIR" ]; then
    find "$TESTDATA_DIR" -type f -delete
fi

echo ">> Running Supply Chain backend..."
pushd "$BACKEND_SRC" >/dev/null

# Compile and run backend engine.
g++ -std=c++11 main.cpp -o engine
echo ">> Backend compiled. Starting engine..."
echo "--------------------------------------------------------"
./engine
popd >/dev/null

echo ""

# Check if suggestions files were generated.
if [ ! "$(ls -A "$SUGGESTIONS_DIR"/*.json 2>/dev/null)" ]; then
    echo "[WARNING] No suggestion JSON files found in Backend/suggestions/"
    echo "[WARNING] Backend may have encountered an error or no recovery routes exist."
    echo "[INFO] Skipping frontend start."
    exit 1
fi

echo ">> Starting frontend dev server..."
pushd "$FRONTEND_DIR" >/dev/null

# Install dependencies if needed.
if [ ! -d "node_modules" ]; then
    echo ">> Installing frontend dependencies..."
    npm install
fi

# Start Vite dev server in background.
npm run dev &
FRONTEND_PID=$!

# Give the dev server a moment to start.
sleep 2

# Try to open the browser on localhost.
if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "http://localhost:3000" >/dev/null 2>&1 || true
elif command -v open >/dev/null 2>&1; then
    open "http://localhost:3000" >/dev/null 2>&1 || true
fi

echo ">> Frontend running at http://localhost:3000"
echo ">> Press Ctrl+C to stop the frontend server."

# Wait for frontend process so script stays alive.
wait $FRONTEND_PID
popd >/dev/null