# Supply Chain Optimization

![Supply Chain Banner](assets/banner.png)

A complete **Supply Chain Optimization** simulator combining a **C++ backend** (graph analytics + route optimization) and a **React frontend** (interactive network visualization).

---

## Repository Metadata

- **Repository name**: `supply-chain`
- **Project title**: Supply Chain Optimization
- **Suggested GitHub description**: C++ implementation of supply chain optimization with failure impact analysis, multi-strategy route planning, and React-based visualization.

---

## Table of Contents

- [What this project does](#what-this-project-does)
- [Key capabilities](#key-capabilities)
- [Tech stack](#tech-stack)
- [Workspace layout](#workspace-layout)
- [Prerequisites](#prerequisites)
- [Quick start](#quick-start)
- [Detailed setup](#detailed-setup)
- [Usage guide](#usage-guide)
- [Input model](#input-model)
- [Optimization strategies](#optimization-strategies)
- [Generated outputs](#generated-outputs)
- [Troubleshooting](#troubleshooting)
- [GitHub publishing](#github-publishing)
- [Additional docs](#additional-docs)
- [License](#license)

---

## What this project does

Given a supply-chain graph in JSON format, the system performs:

1. Graph loading and edge metric computation
2. Failure-node simulation
3. Impact propagation analysis using BFS
4. Inventory-loss reporting
5. Multi-strategy recovery route optimization
6. Route/impact visualization in browser

This makes it suitable for resilience analysis, what-if scenario testing, and strategy comparison.

---

## Key capabilities

- Directed weighted graph simulation
- Failure impact analysis (`source -> dependent path` disruption)
- Recovery planning strategies:
  - Most Optimal
  - Max Supply
  - Min Cost
  - Min Time
  - Run All
- Exported machine-readable JSON strategy outputs
- Interactive UI with highlighted recovery links and metrics
- Quick scenario switching with a CLI helper script

---

## Tech stack

### Backend

- C++11
- `g++`
- `nlohmann/json` single-header parser

### Frontend

- React 19
- TypeScript
- Vite
- D3
- Tailwind utilities + motion UI components

### Automation

- GitHub Actions CI workflow for backend build + frontend typecheck/build

---

## Workspace layout

- [Backend](Backend)
  - [Backend/src/main.cpp](Backend/src/main.cpp) — orchestration pipeline
  - [Backend/src/graph.cpp](Backend/src/graph.cpp) — graph/data loader and state
  - [Backend/src/tree.cpp](Backend/src/tree.cpp) — BFS impact tree
  - [Backend/src/dijkstra.cpp](Backend/src/dijkstra.cpp) — strategy route search
  - [Backend/src/metrics.cpp](Backend/src/metrics.cpp) — route/loss metrics
  - [Backend/src/output.cpp](Backend/src/output.cpp) — report and suggestion writers
  - [Backend/helper/json.hpp](Backend/helper/json.hpp) — JSON library
  - [Backend/suggestions](Backend/suggestions) — generated strategy files
  - [Backend/testdata](Backend/testdata) — generated inventory loss output
- [Frontend](Frontend)
  - [Frontend/src/App.tsx](Frontend/src/App.tsx) — visualization logic
  - [Frontend/src/main.tsx](Frontend/src/main.tsx) — entrypoint
- [input_data](input_data)
  - [input_data/input.json](input_data/input.json) — active input file
  - scenario case files (`case_*.json`)
- [run.sh](run.sh) — full-stack run script
- [use_case.sh](use_case.sh) — scenario selector script
- [assets](assets) — images/media for repository docs

---

## Prerequisites

- Linux/macOS shell
- `g++` with C++11 support
- Node.js 18+
- npm

Recommended checks:

```bash
g++ --version
node --version
npm --version
```

---

## Quick start

From project root:

```bash
./run.sh
```

This script:

1. Cleans old generated JSON outputs
2. Compiles backend engine
3. Runs backend strategy flow in terminal
4. Starts frontend on `http://localhost:3000`

---

## Detailed setup

See full installation and run variants in [SETUP.md](SETUP.md).

---

## Usage guide

See complete workflow guide in [USAGE.md](USAGE.md).

Typical flow:

1. Choose a scenario with [use_case.sh](use_case.sh)
2. Run backend and select strategy
3. Open frontend and inspect highlighted routes
4. Compare generated JSON outputs

---

## Input model

Active file: [input_data/input.json](input_data/input.json)

Expected top-level keys:

- `config`
- `nodes`
- `roads`
- `simulation`

Scenario catalog: [input_data/case_catalog.md](input_data/case_catalog.md)

Template file: [input_data/template.json](input_data/template.json)

---

## Optimization strategies

Implemented in [Backend/src/dijkstra.cpp](Backend/src/dijkstra.cpp) and orchestrated in [Backend/src/main.cpp](Backend/src/main.cpp):

1. **Most Optimal** — composite edge scoring
2. **Max Supply** — widest path by bottleneck capacity
3. **Min Cost** — minimum INR route
4. **Min Time** — minimum total minutes
5. **Run All** — compare all strategy outputs

---

## Generated outputs

### Strategy output files

Generated under [Backend/suggestions](Backend/suggestions):

- `most_optimal.json`
- `max_supply.json`
- `min_cost.json`
- `min_time.json`

### Inventory loss report

Generated at [Backend/testdata/output.json](Backend/testdata/output.json)

---

## Troubleshooting

### 1) `input_data/input.json not found`

- Ensure you run commands from repository root
- Ensure [input_data/input.json](input_data/input.json) exists

### 2) Frontend does not start

```bash
cd Frontend
npm install
npm run dev
```

### 3) No suggestions generated

- Verify backend completed without fatal errors
- Ensure selected scenario has recoverable routes
- Check [Backend/suggestions](Backend/suggestions) for files

### 4) Permission error running scripts

```bash
chmod +x run.sh use_case.sh
```

---

## GitHub publishing

Use the full publish checklist in [GITHUB_PUBLISH.md](GITHUB_PUBLISH.md).

Quick commands:

```bash
git init
git add .
git commit -m "Initial commit: Supply Chain Optimization"
git branch -M main
git remote add origin <your-repo-url>
git push -u origin main
```

---

## Additional docs

- [SETUP.md](SETUP.md)
- [USAGE.md](USAGE.md)
- [ARCHITECTURE.md](ARCHITECTURE.md)
- [Backend/README.md](Backend/README.md)
- [Frontend/README.md](Frontend/README.md)
- [GITHUB_PUBLISH.md](GITHUB_PUBLISH.md)
- [CONTRIBUTING.md](CONTRIBUTING.md)
- [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md)
- [SECURITY.md](SECURITY.md)
- [CHANGELOG.md](CHANGELOG.md)

---

## License

MIT License. See [LICENSE](LICENSE).
