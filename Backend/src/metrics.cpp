// metrics.cpp
// -----------------------------------------------------------------------------
// Route and inventory metrics for src1.
//
// MODULE PURPOSE
// --------------
// 1) Build inventory loss report JSON for failed/offline nodes.
// 2) Compute route totals (cost, time, bottleneck capacity, composite score).
//
// EXAMPLE LOSS REPORT OUTPUT (shape)
// ----------------------------------
// {
//   "affected_nodes": 1,
//   "entries": [
//     {"node":"A","item":"Panels","quantity_lost":100,"unit":"units"}
//   ],
//   "totals_by_unit": {"units": 100}
// }
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>
#include "../helper/json.hpp"

using namespace std;
using json = nlohmann::json;

json loss_report(const unordered_set<string> &off)
{
    // Flat list of loss entries for all offline nodes.
    json entries = json::array();
    // Unit-wise aggregation bucket (e.g., units, kg, pallets).
    unordered_map<string, double> totals;

    // Collect inventory loss from each offline node.
    for (const string &node : off)
    {
        if (!stock.count(node))
            continue;

        for (const auto &item : stock[node])
        {
            json row;
            row["node"] = node;
            row["item"] = item.item;
            row["quantity_lost"] = item.quantity;
            row["unit"] = item.unit;
            entries.push_back(row);
            totals[item.unit] += item.quantity;
        }
    }

    json total_json;
    for (const auto &pair : totals)
        total_json[pair.first] = pair.second;

    // Final report envelope expected by frontend and logs.
    json out;
    out["affected_nodes"] = off.size();
    out["entries"] = entries;
    out["totals_by_unit"] = total_json;
    return out;
}

// route_metrics()
// ---------------
// Computes accumulated route statistics from node path.
//
// Example path: [A, B, C]
// The function finds edges A->B and B->C, then aggregates:
// - cost = cost(A->B) + cost(B->C)
// - time = time(A->B) + time(B->C)
// - capacity = min(cap(A->B), cap(B->C))
// - score = cost + time*20 - capacity*2
RouteResult route_metrics(const vector<string> &path)
{
    RouteResult res;
    res.path = path;
    res.capacity = 1e9;

    for (size_t i = 0; i + 1 < path.size(); i++)
    {
        const string &u = path[i];
        const string &v = path[i + 1];

        for (const auto &edge : graph[u])
        {
            if (edge.to == v)
            {
                res.cost += edge.cost_inr;
                res.time += edge.time_mins;
                res.capacity = min(res.capacity, edge.capacity);
                break;
            }
        }
    }

    res.score = res.cost + (res.time * 20.0) - (res.capacity * 2.0);
    return res;
}