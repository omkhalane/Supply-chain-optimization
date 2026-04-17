# Architecture Overview

## 1) High-level design

The project uses a two-layer architecture:

1. **Backend engine (C++)**
2. **Frontend visualizer (React + D3)**

The integration contract is file-based JSON output.

---

## 2) Backend modules

Source folder: [Backend/src](Backend/src)

- [Backend/src/main.cpp](Backend/src/main.cpp)
  - End-to-end pipeline orchestration
  - Failure injection, strategy selection, and output generation

- [Backend/src/graph.cpp](Backend/src/graph.cpp)
  - Data structures (`Edge`, `RouteResult`, inventory)
  - JSON parsing and graph construction
  - Source/sink derivation

- [Backend/src/tree.cpp](Backend/src/tree.cpp)
  - BFS impact tree generation

- [Backend/src/dijkstra.cpp](Backend/src/dijkstra.cpp)
  - Multi-mode route computation:
    - shortest path variants
    - widest path for capacity mode

- [Backend/src/metrics.cpp](Backend/src/metrics.cpp)
  - Route metrics and inventory loss calculations

- [Backend/src/output.cpp](Backend/src/output.cpp)
  - JSON output persistence
  - Console result formatting

---

## 3) Frontend modules

Source folder: [Frontend/src](Frontend/src)

- [Frontend/src/main.tsx](Frontend/src/main.tsx)
  - App bootstrap

- [Frontend/src/App.tsx](Frontend/src/App.tsx)
  - Graph rendering
  - Failure/impact styling
  - Strategy output overlays
  - Interactive route/node details

---

## 4) Runtime data flow

1. Input loaded from [input_data/input.json](input_data/input.json)
2. Backend computes impacts and optimized routes
3. Backend writes:
   - [Backend/testdata/output.json](Backend/testdata/output.json)
   - [Backend/suggestions](Backend/suggestions)
4. Frontend fetches input + outputs and visualizes results

---

## 5) Strategy model summary

- **Most Optimal**: weighted composite score
- **Max Supply**: maximize bottleneck capacity
- **Min Cost**: minimize total INR
- **Min Time**: minimize travel minutes

---

## 6) Reliability notes

- Backend handles malformed existing output JSON by resetting output shape
- Missing optional road fields are defaulted (`capacity`, `tollTax`, `trafficDelay`)
- `run.sh` cleans generated files before each new run

---

## 7) Extension ideas

- Split C++ include-based composition into `.hpp` + `.cpp` units
- Add unit tests for route correctness per strategy
- Add JSON schema validation for input format
- Add scenario comparison mode in frontend
