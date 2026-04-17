// output.cpp
// -----------------------------------------------------------------------------
// Output helpers for src1.
//
// RESPONSIBILITIES
// ----------------
// save_loss(report):
//   Appends inventory loss report to ../testdata/output.json under
//   "inventory_losses" array.
//
// show_result(...):
//   Prints selected strategy summary to console and writes strategy JSON to
//   ../suggestions/<key>.json.
//
// EXAMPLE STRATEGY FILE
// ---------------------
// ../suggestions/min_cost.json
// {
//   "strategy": "MIN COST",
//   "path": ["A","B","C"],
//   "cost": 1234.5,
//   "time": 180,
//   "capacity": 800,
//   "implemented_links": ["A-B","B-C"],
//   "implemented_optimization_routes": [...],
//   "unrecoverable_nodes": ["X"]
// }
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>
#include "../helper/json.hpp"

using namespace std;
using json = nlohmann::json;

void save_loss(const json &report)
{
    // Central output file for cumulative inventory-loss logs.
    string file_name = "../testdata/output.json";
    json data;

    // Read existing output if available; if corrupt, reset safely.
    ifstream in(file_name);
    if (in)
    {
        try
        {
            in >> data;
        }
        catch (...)
        {
            data = json::object();
        }
    }

    // Normalize shape before append.
    if (!data.is_object())
        data = json::object();

    if (!data.contains("inventory_losses") || !data["inventory_losses"].is_array())
        data["inventory_losses"] = json::array();

    // Append current run report.
    data["inventory_losses"].push_back(report);

    ofstream out(file_name);
    out << data.dump(4);
}

// show_result()
// -------------
// Emits both:
// 1) Human-readable console summary
// 2) Machine-readable strategy JSON file
//
// Parameters:
// - name: display strategy name
// - res: best route result for that strategy
// - key: output filename stem (e.g., "min_cost")
// - plan: per-reactivated-node route plan array
// - bad: list/json array of unrecoverable nodes
void show_result(const string &name, const RouteResult &res, const string &key,
                 const json &plan, const json &bad)
{
    if (res.path.empty())
    {
        cout << "[\033[31mFATAL\033[0m] No alternative routes available for " << name << ".\n";
        return;
    }

    cout << ">> Strategy Executed: " << name << "\n";
    cout << ">> Active Recovery Route:\n   ";
    for (size_t i = 0; i < res.path.size(); i++)
        cout << "[" << res.path[i] << "]" << (i + 1 == res.path.size() ? "" : " ===> ");

    cout << "\n\n>> Route Statistics:\n";
    cout << "   - Total Cost: Rs. " << fixed << setprecision(2) << res.cost << "\n";
    cout << "   - Total Time: " << res.time << " mins (~" << fixed << setprecision(1) << (res.time / 60.0) << " hours)\n";
    cout << "   - Bottleneck Capacity: " << res.capacity << " units\n";

    // Build unique implemented link list across all plan steps.
    json links = json::array();
    unordered_set<string> seen;
    for (const auto &step : plan)
    {
        if (!step.contains("links"))
            continue;

        for (const auto &link : step["links"])
        {
            string s = link.get<string>();
            if (!seen.count(s))
            {
                seen.insert(s);
                links.push_back(s);
            }
        }
    }

    cout << "\n>> Implemented Optimization Links (A-B):\n";
    if (links.empty())
        cout << "   - None\n";
    else
        for (size_t i = 0; i < links.size(); i++)
            cout << "   - " << links[i].get<string>() << "\n";

    if (!bad.empty())
    {
        cout << "\n>> Unrecoverable Dependent Nodes (kept inactive): ";
        for (size_t i = 0; i < bad.size(); i++)
            cout << bad[i].get<string>() << (i + 1 == bad.size() ? "" : ", ");
        cout << "\n";
    }

    cout << "--------------------------------------------------------\n";

    // Output payload consumed by frontend strategy panel.
    json out;
    out["strategy"] = name;
    out["path"] = res.path;
    out["cost"] = res.cost;
    out["time"] = res.time;
    out["capacity"] = res.capacity;
    out["implemented_links"] = links;
    out["implemented_optimization_routes"] = plan;
    out["unrecoverable_nodes"] = bad;

    string file_name = "../suggestions/" + key + ".json";
    ofstream out_file(file_name);
    out_file << out.dump(4);
}