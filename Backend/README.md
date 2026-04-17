# Backend (C++)

This backend executes supply chain failure simulation and recovery route optimization.

## Main entry

- [src/main.cpp](src/main.cpp)

## Modules

- [src/graph.cpp](src/graph.cpp): graph + inventory loading
- [src/tree.cpp](src/tree.cpp): BFS impact tree
- [src/dijkstra.cpp](src/dijkstra.cpp): strategy route search
- [src/metrics.cpp](src/metrics.cpp): metrics computation
- [src/output.cpp](src/output.cpp): report/suggestion JSON output

## Build

From [Backend/src](src):

```bash
g++ -std=c++11 main.cpp -o engine
./engine
```

## Outputs

- Suggestions: [suggestions](suggestions)
- Loss report: [testdata/output.json](testdata/output.json)

## JSON dependency

- [helper/json.hpp](helper/json.hpp)
- Helper guide: [helper/README.md](helper/README.md)
