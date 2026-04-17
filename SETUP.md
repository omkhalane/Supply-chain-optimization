# Setup Guide

This guide covers clean setup for **Supply Chain Optimization**.

## 1) System requirements

- Linux or macOS shell
- C++ compiler (`g++`, C++11+)
- Node.js 18+
- npm

Validate tools:

```bash
g++ --version
node --version
npm --version
```

---

## 2) Clone repository

```bash
git clone <your-repo-url>
cd supply-chain
```

---

## 3) Backend setup (C++)

Backend source location: [Backend/src](Backend/src)

Build manually:

```bash
cd Backend/src
g++ -std=c++11 main.cpp -o engine
./engine
```

Return to root when done:

```bash
cd ../..
```

---

## 4) Frontend setup (React)

Frontend location: [Frontend](Frontend)

Install and run:

```bash
cd Frontend
npm install
npm run dev
```

Frontend defaults to:

- a dynamic available Vite port (commonly `http://localhost:5173`)

---

## 5) Full-stack setup (recommended)

From root, run:

```bash
./run.sh
```

What this does:

1. Cleans old generated files
2. Compiles backend and runs optimization flow
3. Starts frontend server only if suggestions/routes are generated
4. Detects and opens whichever frontend URL/port Vite assigns

---

## 6) Scenario switching setup

Use [use_case.sh](use_case.sh) to swap active test scenario.

List available cases:

```bash
./use_case.sh --list
```

Activate one case and run full flow:

```bash
./use_case.sh case_simple_linear.json
```

---

## 7) Common setup issues

### Script permission denied

```bash
chmod +x run.sh use_case.sh
```

### npm install fails

- Remove lock + node_modules and reinstall:

```bash
cd Frontend
rm -rf node_modules package-lock.json
npm install
```

### C++ compile fails

- Ensure `g++` is installed and supports C++11.

### Frontend files not pushed to GitHub

If `Frontend` was an embedded repository, convert it to regular tracked files:

```bash
git rm --cached Frontend
rm -rf Frontend/.git
git add Frontend
```

---

## 8) Next step

After setup, follow [USAGE.md](USAGE.md) for full simulation workflow and output interpretation.
