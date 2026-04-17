# `json.hpp` Usage Guide (Backend/helper)

This project uses **[nlohmann/json](https://github.com/nlohmann/json)** via the single-header file:

- `Backend/helper/json.hpp`

In code, it is usually aliased as:

```cpp
#include "../helper/json.hpp"
using json = nlohmann::json;
```

---

## 1) What `json.hpp` gives you

With one header, you can:

- Parse JSON from files/strings
- Read values safely with defaults
- Build JSON objects/arrays in C++
- Serialize (pretty print) to files
- Validate expected fields before processing

---

## 2) Include patterns in this project

From files inside `Backend/src`, use:

```cpp
#include "../helper/json.hpp"
using json = nlohmann::json;
```

From files in the same folder as `json.hpp`, use:

```cpp
#include "json.hpp"
using json = nlohmann::json;
```

---

## 3) Parse JSON from a file

```cpp
#include <fstream>
#include <iostream>
#include "../helper/json.hpp"
using json = nlohmann::json;

int main() {
    std::ifstream file("../../input_data/input.json");
    if (!file) {
        std::cout << "input file not found\n";
        return 1;
    }

    json data;
    file >> data;  // parse directly from stream

    std::cout << "Loaded successfully\n";
    return 0;
}
```

---

## 4) Parse JSON from a string

```cpp
#include <iostream>
#include <string>
#include "../helper/json.hpp"
using json = nlohmann::json;

int main() {
    std::string raw = R"({"name":"Plant-A","capacity":120})";
    json j = json::parse(raw);

    std::cout << j["name"] << " | " << j["capacity"] << "\n";
}
```

---

## 5) Safely reading fields (recommended)

Use `.contains()` and `.value()` to avoid crashes when keys are missing.

```cpp
std::string nodeId = node.value("id", "");
int capacity = road.contains("capacity") ? (int)road["capacity"] : 100;
int delay = road.value("trafficDelay", 0);
```

### Why this matters

- `j["missing"]` may create/assume key unexpectedly.
- `j.at("missing")` throws an exception.
- `j.value("missing", default)` is best when defaults are acceptable.

---

## 6) Reading arrays

```cpp
if (data.contains("roads") && data["roads"].is_array()) {
    for (const auto& road : data["roads"]) {
        std::string from = road.value("from", "");
        std::string to = road.value("to", "");
        int dist = road.value("distance", 0);

        if (from.empty() || to.empty() || dist <= 0) {
            continue;  // skip invalid row
        }

        // process road...
    }
}
```

---

## 7) Building JSON objects and arrays

```cpp
json report;
report["status"] = "ok";
report["affected_nodes"] = 3;

json entries = json::array();
entries.push_back({
    {"node", "W1"},
    {"item", "Rice"},
    {"quantity_lost", 120.0},
    {"unit", "kg"}
});
entries.push_back({
    {"node", "W2"},
    {"item", "Sugar"},
    {"quantity_lost", 75.0},
    {"unit", "kg"}
});

report["entries"] = entries;
```

---

## 8) Writing JSON to disk (pretty format)

```cpp
#include <fstream>

std::ofstream out("../../Backend/suggestions/min_cost.json");
out << report.dump(4);  // 4-space indentation
```

- `dump()` gives minified JSON
- `dump(4)` gives pretty, readable JSON

---

## 9) Handling parse/type errors

`json.hpp` throws exceptions for malformed JSON and wrong type conversions.

```cpp
try {
    json j = json::parse(rawInput);
    int x = j.at("someNumber").get<int>();
} catch (const json::parse_error& e) {
    std::cerr << "JSON parse error: " << e.what() << "\n";
} catch (const json::type_error& e) {
    std::cerr << "JSON type error: " << e.what() << "\n";
} catch (const json::out_of_range& e) {
    std::cerr << "Missing key/index: " << e.what() << "\n";
}
```

---

## 10) Project-style input validation helper

Use this pattern before graph loading:

```cpp
bool validateInputShape(const json& data, std::string& reason) {
    if (!data.contains("config") || !data["config"].is_object()) {
        reason = "Missing or invalid 'config' object";
        return false;
    }

    if (!data.contains("roads") || !data["roads"].is_array()) {
        reason = "Missing or invalid 'roads' array";
        return false;
    }

    for (const auto& road : data["roads"]) {
        if (!road.contains("from") || !road.contains("to") || !road.contains("distance")) {
            reason = "Each road must contain from, to, distance";
            return false;
        }
    }

    return true;
}
```

---

## 11) Handy conversion APIs

```cpp
std::string name = j.at("name").get<std::string>();
double cost = j.value("cost", 0.0);
auto arr = j.at("items");               // json array
std::vector<int> v = arr.get<std::vector<int>>();
```

For custom structs, you can add serializer functions:

```cpp
struct EdgeDTO {
    std::string from;
    std::string to;
    int distance;
};

void to_json(json& j, const EdgeDTO& e) {
    j = json{{"from", e.from}, {"to", e.to}, {"distance", e.distance}};
}

void from_json(const json& j, EdgeDTO& e) {
    e.from = j.value("from", "");
    e.to = j.value("to", "");
    e.distance = j.value("distance", 0);
}
```

---

## 12) Common mistakes and fixes

### Mistake 1: Assuming keys always exist

```cpp
int d = road["distance"]; // risky
```

✅ Better:

```cpp
int d = road.value("distance", 0);
```

### Mistake 2: Assuming correct type without checks

```cpp
int delay = road["trafficDelay"].get<int>(); // throws if string/null
```

✅ Better:

```cpp
int delay = 0;
if (road.contains("trafficDelay") && road["trafficDelay"].is_number_integer()) {
    delay = road["trafficDelay"].get<int>();
}
```

### Mistake 3: Forgetting to check file open

```cpp
std::ifstream f("input.json");
json j; f >> j; // fails badly if file missing
```

✅ Better:

```cpp
std::ifstream f("input.json");
if (!f) {
    // handle missing file
}
```

---

## 13) Mini end-to-end example (read -> process -> write)

```cpp
#include <fstream>
#include <iostream>
#include "../helper/json.hpp"
using json = nlohmann::json;

int main() {
    std::ifstream in("../../input_data/input.json");
    if (!in) {
        std::cerr << "Cannot open input file\n";
        return 1;
    }

    json data;
    in >> data;

    int roadsCount = 0;
    if (data.contains("roads") && data["roads"].is_array()) {
        roadsCount = (int)data["roads"].size();
    }

    json out;
    out["summary"] = {
        {"roads_count", roadsCount},
        {"has_config", data.contains("config")}
    };

    std::ofstream result("../../Backend/testdata/output.json");
    result << out.dump(4);

    std::cout << "Done. output.json written\n";
    return 0;
}
```

---

## 14) Quick best-practice checklist

- Prefer `value(key, default)` for optional fields.
- Use `contains()` + `is_*()` checks before strict conversions.
- Validate required top-level shape early (`config`, `roads`, etc.).
- Use `dump(4)` when writing logs/suggestion JSON for readability.
- Catch parse/type exceptions at boundaries (input loading).

---

## 15) License note

`json.hpp` is from **nlohmann/json** (MIT License). Keep the original header/license notices intact.

If you update the header in future, use the official source:

- https://github.com/nlohmann/json
