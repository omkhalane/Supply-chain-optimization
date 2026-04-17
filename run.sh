#!/bin/bash

# -----------------------------------------------------------------------------
# Run the full stack locally:
# 1) Compile and run backend engine (C++).
# 2) Start frontend dev server (Vite).
# 3) Open whichever local URL/port Vite assigns.
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
    echo "[WARNING] No recoverable routes may exist for this scenario."
    echo "[INFO] Frontend will not be started because no route/suggestion is available."
    exit 0
fi

echo ">> Starting frontend dev server..."
pushd "$FRONTEND_DIR" >/dev/null

# Install dependencies if needed.
if [ ! -d "node_modules" ]; then
    echo ">> Installing frontend dependencies..."
    npm install
fi

# Start Vite dev server in background and capture logs.
VITE_LOG_FILE="$(mktemp)"
npm run dev >"$VITE_LOG_FILE" 2>&1 &
FRONTEND_PID=$!

# Wait until Vite prints a local URL.
FRONTEND_URL=""
for _ in {1..30}; do
    if ! kill -0 "$FRONTEND_PID" >/dev/null 2>&1; then
        break
    fi

    FRONTEND_URL="$(grep -Eo 'http://(localhost|127\.0\.0\.1):[0-9]+' "$VITE_LOG_FILE" | head -n 1 || true)"
    if [ -n "$FRONTEND_URL" ]; then
        break
    fi

    sleep 1
done

# If process already exited before startup is complete, fail with logs.
if ! kill -0 "$FRONTEND_PID" >/dev/null 2>&1; then
    echo "[ERROR] Frontend failed to start."
    echo "[ERROR] Vite output:"
    cat "$VITE_LOG_FILE"
    exit 1
fi

# Fallback if URL was not parsed but process is alive.
if [ -z "$FRONTEND_URL" ]; then
    FRONTEND_URL="http://localhost:5173"
fi

# Try to open the browser on localhost.
if command -v xdg-open >/dev/null 2>&1; then
    xdg-open "$FRONTEND_URL" >/dev/null 2>&1 || true
elif command -v open >/dev/null 2>&1; then
    open "$FRONTEND_URL" >/dev/null 2>&1 || true
fi

echo ">> Frontend running at $FRONTEND_URL"
echo ">> Press Ctrl+C to stop the frontend server."

# Stream frontend logs while script is running.
tail -f "$VITE_LOG_FILE" &
TAIL_PID=$!

# Wait for frontend process so script stays alive.
wait $FRONTEND_PID
kill "$TAIL_PID" >/dev/null 2>&1 || true
rm -f "$VITE_LOG_FILE"
popd >/dev/null