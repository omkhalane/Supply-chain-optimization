# Usage Guide

This guide explains how to run simulations, choose strategies, and read outputs.

## 1) Prepare input

Active input file:

- [input_data/input.json](input_data/input.json)

You can either:

- Edit it directly, or
- Activate a predefined case using [use_case.sh](use_case.sh)

---

## 2) Run a simulation

### Option A: one command

```bash
./run.sh
```

### Option B: case + run

```bash
./use_case.sh case_simple_branching.json
```

### Option C: interactive case selection

```bash
./use_case.sh
```

---

## 3) Strategy selection in backend

When prompted, enter one of:

1. Most Optimal
2. Max Supply
3. Min Cost
4. Min Time
5. Run All

The backend then computes recovery routes from healthy sources to impacted nodes/sinks.

---

## 4) Review frontend

When suggestions are available, [run.sh](run.sh) starts frontend automatically and prints the detected URL/port.

Common local URL example:

- `http://localhost:5173`

If no suggestions are generated, frontend is skipped by design.
You can still run frontend manually:

```bash
cd Frontend
npm run dev
```

Key frontend behaviors:

- Shows failed and impacted nodes
- Highlights optimization links (green)
- Displays route metrics on optimized edges
- Loads generated JSON from backend output folders

---

## 5) Understand output files

### Strategy outputs

Location: [Backend/suggestions](Backend/suggestions)

Files:

- `most_optimal.json`
- `max_supply.json`
- `min_cost.json`
- `min_time.json`

Common fields:

- `strategy`
- `path`
- `cost`
- `time`
- `capacity`
- `implemented_links`
- `implemented_optimization_routes`
- `unrecoverable_nodes`

### Inventory loss output

Location: [Backend/testdata/output.json](Backend/testdata/output.json)

Contains `inventory_losses` history entries.

---

## 6) Work with test catalog

Catalog reference:

- [input_data/case_catalog.md](input_data/case_catalog.md)

Useful commands:

```bash
./use_case.sh --list
./use_case.sh case_big_dense_network.json
./use_case.sh case_edge_disconnected_components.json
```

---

## 7) Recommended analysis workflow

1. Start from a simple case
2. Note impacted nodes after failure
3. Run each strategy and compare outputs
4. Track unrecoverable nodes per strategy
5. Validate business trade-offs:
   - Capacity vs Cost vs Time

---

## 8) Reset and rerun

`run.sh` clears generated output before each run. If needed, rerun from root:

```bash
./run.sh
```
