// graph.cpp
// -----------------------------------------------------------------------------
// Simple shared graph/data module for the src1 codebase.
//
// WHY THIS FILE EXISTS
// --------------------
// This file is the central data + loader layer for the simulation.
// It defines:
// 1) Core data structures (Edge, RouteResult, InventoryItem)
// 2) Global in-memory state used by other modules
// 3) JSON -> graph transformation logic
// 4) Console visualization helper for the current network state
//
// INPUT EXPECTATION (high level)
// ------------------------------
// The loader expects a JSON document like:
// {
//   "config": {
//     "petrolRate_INR": 102,
//     "truckMileage_kmpl": 5
//   },
//   "nodes": [
//     {
//       "id": "Display_Tech_Corp",
//       "inventory": [
//         {"item": "Panels", "quantity": 1000, "unit": "units"}
//       ]
//     }
//   ],
//   "roads": [
//     {
//       "from": "Display_Tech_Corp",
//       "to": "Shenzhen_Assembly_Plant",
//       "distance": 200,
//       "capacity": 2000,
//       "tollTax": 0,
//       "trafficDelay": 10
//     }
//   ]
// }
//
// IMPORTANT NOTE ABOUT GLOBALS
// ----------------------------
// This project uses include-based composition (main.cpp includes module .cpp
// files directly). Therefore, globals declared here are visible to all modules
// included after this file in main.cpp.
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>
#include "../helper/json.hpp"

using namespace std;
using json = nlohmann::json;

struct Edge
{
    // Destination node id for directed edge: u -> to
    string to;
    // Distance in km from input
    int distance;
    // Precomputed travel time in minutes (distance + delay)
    int time_mins;
    // Precomputed cost in INR for traversing this edge
    double cost_inr;
    // Max transferable units over this edge
    int capacity;
};

struct RouteResult
{
    // Ordered list of nodes along selected route
    vector<string> path;
    // Sum of edge costs
    double cost = 0.0;
    // Sum of edge time values
    int time = 0;
    // Path bottleneck = minimum edge capacity on route
    int capacity = -1;
    // Composite score (used by "Most Optimal")
    double score = 1e9;
};

struct InventoryItem
{
    // SKU/material name
    string item;
    // Quantity available
    double quantity;
    // Unit label (e.g., units, kg, pallets)
    string unit;
};

// GLOBAL RUNTIME STATE
// --------------------
// graph[u] gives all outgoing edges from node u.
unordered_map<string, vector<Edge>> graph;
// Universe of all node ids seen in roads.
unordered_set<string> all_nodes;
// Source nodes: has outgoing edges but no incoming edges.
unordered_set<string> sources;
// Sink nodes: has incoming edges but no outgoing edges.
unordered_set<string> sinks;
// Stock map: node -> inventory entries.
unordered_map<string, vector<InventoryItem>> stock;

// load_graph()
// ------------
// Converts JSON input into in-memory graph + inventory + source/sink sets.
//
// DETAILED BEHAVIOR
// - Validates that config and roads exist.
// - Reads optional node inventory section.
// - Computes each road's:
//   time_mins = distance + trafficDelay
//   cost_inr  = (distance / mileage) * petrol_rate + toll
// - Populates graph adjacency list.
// - Derives sources/sinks from in-degree / out-degree properties.
//
// MINI EXAMPLE
// If road is: distance=100, mileage=5, petrol_rate=100, toll=50, delay=20
// then:
//   time_mins = 100 + 20 = 120
//   cost_inr  = (100/5)*100 + 50 = 2050
void load_graph(const json &data)
{
    if (!data.contains("config") || !data.contains("roads"))
    {
        cout << "[!] ERROR: Mandatory fields missing in input.json\n";
        exit(1);
    }

    // Optional inventory block. If absent, stock remains empty.
    if (data.contains("nodes"))
    {
        for (const auto &node : data["nodes"])
        {
            // Safe read; skip malformed node entries.
            string id = node.value("id", "");
            if (id.empty() || !node.contains("inventory"))
                continue;

            for (const auto &item_data : node["inventory"])
            {
                InventoryItem item;
                item.item = item_data.value("item", "Unknown");
                item.quantity = item_data.value("quantity", 0.0);
                item.unit = item_data.value("unit", "units");
                stock[id].push_back(item);
            }
        }
    }

    // Cost model parameters (global for all roads).
    double petrol_rate = data["config"]["petrolRate_INR"];
    double mileage = data["config"]["truckMileage_kmpl"];

    unordered_set<string> has_in;
    unordered_set<string> has_out;

    // Parse all directed roads and build adjacency list.
    for (const auto &road : data["roads"])
    {
        string from = road["from"];
        string to = road["to"];
        int distance = road["distance"];

        // Optional fields with defaults when missing.
        int capacity = road.contains("capacity") ? (int)road["capacity"] : 100;
        int toll = road.contains("tollTax") ? (int)road["tollTax"] : 0;
        int delay = road.contains("trafficDelay") ? (int)road["trafficDelay"] : 0;

        int time_mins = distance + delay;
        double cost_inr = ((distance / mileage) * petrol_rate) + toll;

        graph[from].push_back({to, distance, time_mins, cost_inr, capacity});
        all_nodes.insert(from);
        all_nodes.insert(to);
        has_out.insert(from);
        has_in.insert(to);
    }

    // Classify every node as source/sink using degree info.
    for (const string &node : all_nodes)
    {
        if (has_out.count(node) && !has_in.count(node))
            sources.insert(node);
        if (has_in.count(node) && !has_out.count(node))
            sinks.insert(node);
    }
}

// show_graph()
// ------------
// Prints the graph state to console.
// - OFFLINE nodes are rendered with red marker.
// - Edges into offline nodes are shown as BROKEN.
// - Active edges show distance/time/cost/capacity.
//
// EXAMPLE OUTPUT (conceptual)
// [ACTIVE]  A
//   ├──> B (Dist: 100km | Time: 120m | Cost: Rs.2050 | Cap: 800)
//   └──> [BROKEN] ~~C~~
void show_graph(const unordered_set<string> &off)
{
    for (const auto &pair : graph)
    {
        string node = pair.first;

        if (off.count(node))
            cout << "[\033[31mOFFLINE\033[0m] ~~" << node << "~~\n";
        else
            cout << "[ACTIVE]  " << node << "\n";

        int edge_count = (int)pair.second.size();
        for (int i = 0; i < edge_count; i++)
        {
            const auto &edge = pair.second[i];
            if (off.count(edge.to))
            {
                cout << "  ├──> [\033[31mBROKEN\033[0m] ~~" << edge.to << "~~\n";
                continue;
            }

            string branch = (i == edge_count - 1) ? "  └──> " : "  ├──> ";
            cout << branch << edge.to
                 << " (Dist: " << edge.distance << "km | Time: " << edge.time_mins
                 << "m | Cost: Rs." << fixed << setprecision(0) << edge.cost_inr
                 << " | Cap: " << edge.capacity << ")\n";
        }

        cout << "\n";
    }
}