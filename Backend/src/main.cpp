// main.cpp
// -----------------------------------------------------------------------------
// Simple entry point for src1.
//
// END-TO-END PIPELINE
// -------------------
// 1) Load input JSON and build graph + inventory state.
// 2) Show initial network.
// 3) Apply failure node and build impacted BFS tree.
// 4) Save inventory loss report.
// 5) Ask operator for optimization strategy.
// 6) Compute best source->sink recovery route(s).
// 7) Build per-node reactivation plan.
// 8) Save strategy outputs and print stabilized network.
//
// STRATEGY IDs
// ------------
// 1 = Most Optimal (composite score)
// 2 = Max Supply   (max bottleneck capacity)
// 3 = Min Cost     (minimum INR)
// 4 = Min Time     (minimum minutes)
// 5 = Run all      (compare all four)
//
// QUICK EXAMPLE RUN
// -----------------
// - input.json has failureNode="Shenzhen_Assembly_Plant"
// - choose strategy 3 (Min Cost)
// - program writes:
//   - ../testdata/output.json (inventory loss append)
//   - ../suggestions/min_cost.json
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>

using namespace std;

#include "graph.cpp"
#include "metrics.cpp"
#include "dijkstra.cpp"
#include "tree.cpp"
#include "output.cpp"

bool can_reactivate(const string &target, int choice, const unordered_set<string> &off)
{
    // Convert UI choice into concrete strategy modes to test.
    vector<int> modes = (choice == 5) ? vector<int>{1, 2, 3, 4} : vector<int>{choice};

    for (const string &src : sources)
    {
        if (off.count(src))
            continue;

        for (int mode : modes)
        {
            vector<string> path = dijkstra(src, target, mode, off);
            if (!path.empty())
                return true;
        }
    }

    // No healthy source can reach this impacted target.
    return false;
}

// best_route()
// ------------
// For a target impacted node and one strategy mode, find the best route from
// any healthy source.
//
// Selection rules:
// - mode 1: lower score is better
// - mode 2: higher capacity is better
// - mode 3: lower cost is better
// - mode 4: lower time is better
RouteResult best_route(const string &target, int mode, const unordered_set<string> &off)
{
    RouteResult best;
    bool found = false;

    best.score = 1e9;
    best.cost = 1e9;
    best.time = 1e9;
    best.capacity = -1;

    for (const string &src : sources)
    {
        if (off.count(src))
            continue;

        vector<string> path = dijkstra(src, target, mode, off);
        if (path.empty())
            continue;

        RouteResult cur = route_metrics(path);
        if (!found)
        {
            best = cur;
            found = true;
            continue;
        }

        if (mode == 1 && cur.score < best.score)
            best = cur;
        else if (mode == 2 && cur.capacity > best.capacity)
            best = cur;
        else if (mode == 3 && cur.cost < best.cost)
            best = cur;
        else if (mode == 4 && cur.time < best.time)
            best = cur;
    }

    if (!found)
        return RouteResult();

    return best;
}

// build_plan()
// ------------
// Creates actionable optimization plan for all impacted nodes.
//
// For each impacted node:
// - if route exists => mark active and store route/cost/time/capacity + links
// - else           => mark as unrecoverable (bad)
//
// Output JSON is consumed by frontend to highlight links and route metrics.
json build_plan(const unordered_set<string> &impact, int mode,
                const unordered_set<string> &off,
                unordered_set<string> &active,
                unordered_set<string> &bad)
{
    vector<string> nodes(impact.begin(), impact.end());
    sort(nodes.begin(), nodes.end());

    json plan = json::array();

    for (const string &node : nodes)
    {
        RouteResult r = best_route(node, mode, off);
        if (r.path.empty())
        {
            bad.insert(node);
            continue;
        }

        active.insert(node);

        json links = json::array();
        for (size_t i = 0; i + 1 < r.path.size(); i++)
            links.push_back(r.path[i] + "-" + r.path[i + 1]);

        json item;
        item["reactivated_node"] = node;
        item["route"] = r.path;
        item["links"] = links;
        item["cost"] = r.cost;
        item["time"] = r.time;
        item["capacity"] = r.capacity;
        plan.push_back(item);
    }

    return plan;
}

int main()
{
    // -----------------------------------------------------------------
    // Step 1: Read input and initialize runtime state.
    // -----------------------------------------------------------------
    ifstream file("../../input_data/input.json");
    if (!file)
    {
        cout << "[!] ERROR: input_data/input.json not found!\n";
        return 0;
    }

    json data;
    file >> data;

    // Build adjacency list graph + source/sink sets + inventory map.
    load_graph(data);

    // Track nodes that are offline in current simulation snapshot.
    unordered_set<string> off;

    // Show baseline network before fault injection.
    cout << "\n========================================================\n";
    cout << "               INITIAL SUPPLY CHAIN NETWORK               \n";
    cout << "========================================================\n\n";
    show_graph(off);

    // Read configured failure node from simulation section.
    string fail = data["simulation"].value("failureNode", "");
    // No valid failure configured => nothing to optimize.
    if (fail.empty() || !graph.count(fail))
    {
        cout << "\n[OK] No critical failures detected. System is STABLE.\n";
        return 0;
    }

    // Mark failure node as offline.
    off.insert(fail);

    // Build BFS impacted tree and impacted node set.
    Tree tree = BFS(fail);
    unordered_set<string> impact = tree.impact;

    // Save inventory loss report for current failure state.
    json loss = loss_report(off);
    save_loss(loss);

    cout << ">> Inventory loss entries identified: " << loss["entries"].size() << "\n\n";
    cout << ">> Nodes initially affected by failure: " << impact.size() << "\n\n";

    // Show post-failure network and impacted tree diagnostics.
    cout << "\n========================================================\n";
    cout << "[\033[31m!\033[0m] CRITICAL ALERT: NODE FAILURE DETECTED\n";
    cout << "    Location: " << fail << "\n";
    cout << "========================================================\n\n";

    cout << "               NETWORK STATE AFTER FAILURE               \n";
    cout << "========================================================\n\n";
    unordered_set<string> after = off;
    after.insert(impact.begin(), impact.end());
    show_graph(after);

    tree_display(tree);

    // -----------------------------------------------------------------
    // Step 2: Strategy selection.
    // -----------------------------------------------------------------
    cout << "\n========================================================\n";
    cout << "      SELECT GLOBAL STABILIZATION STRATEGY      \n";
    cout << "========================================================\n";
    cout << "1. Most Optimal (Safest - Balances Cost/Time/Capacity)\n";
    cout << "2. Max Supply (Prevents stockouts - Prioritizes High Capacity)\n";
    cout << "3. Min Cost (Budget preservation - Lowest INR)\n";
    cout << "4. Min Time (Emergency rush - Fastest Delivery)\n";
    cout << "5. Run All Strategies (Compare all options)\n";
    cout << "Enter choice (1-5): ";

    int choice;
    cin >> choice;

    cout << "\n>> ENGINE: Scanning " << sources.size() << " Suppliers and " << sinks.size() << " Markets...\n";
    cout << ">> GENERATING BYPASS ROUTES...\n\n";

    // -----------------------------------------------------------------
    // Step 3: Evaluate candidate routes for all source->sink pairs.
    // -----------------------------------------------------------------
    RouteResult min_cost_route, max_supply_route, min_time_route, best_opt_route;
    best_opt_route.score = 1e9;
    max_supply_route.capacity = -1;
    min_cost_route.cost = 1e9;
    min_time_route.time = 1e9;

    // Signals whether at least one valid recovery path exists.
    bool path_found = false;

    for (const string &src : sources)
    {
        if (off.count(src))
            continue;

        for (const string &dst : sinks)
        {
            if (off.count(dst))
                continue;

            // Strategy 1: Most Optimal
            if (choice == 1 || choice == 5)
            {
                vector<string> path = dijkstra(src, dst, 1, off);
                if (!path.empty())
                {
                    path_found = true;
                    RouteResult r = route_metrics(path);
                    if (r.score < best_opt_route.score)
                        best_opt_route = r;
                }
            }

            // Strategy 2: Max Supply
            if (choice == 2 || choice == 5)
            {
                vector<string> path = dijkstra(src, dst, 2, off);
                if (!path.empty())
                {
                    path_found = true;
                    RouteResult r = route_metrics(path);
                    if (r.capacity > max_supply_route.capacity)
                        max_supply_route = r;
                }
            }

            // Strategy 3: Min Cost
            if (choice == 3 || choice == 5)
            {
                vector<string> path = dijkstra(src, dst, 3, off);
                if (!path.empty())
                {
                    path_found = true;
                    RouteResult r = route_metrics(path);
                    if (r.cost < min_cost_route.cost)
                        min_cost_route = r;
                }
            }

            // Strategy 4: Min Time
            if (choice == 4 || choice == 5)
            {
                vector<string> path = dijkstra(src, dst, 4, off);
                if (!path.empty())
                {
                    path_found = true;
                    RouteResult r = route_metrics(path);
                    if (r.time < min_time_route.time)
                        min_time_route = r;
                }
            }
        }
    }

    // If no route survives failure constraints, system collapses.
    if (!path_found)
    {
        cout << "[\033[31mFATAL\033[0m] No alternative routes available. System COLLAPSED.\n";
        return 0;
    }

    // -----------------------------------------------------------------
    // Step 4: Build plans and compute final active/inactive status.
    // -----------------------------------------------------------------
    unordered_set<string> active_opt, bad_opt;
    unordered_set<string> active_sup, bad_sup;
    unordered_set<string> active_cost, bad_cost;
    unordered_set<string> active_time, bad_time;

    json plan_opt = json::array();
    json plan_sup = json::array();
    json plan_cost = json::array();
    json plan_time = json::array();

    // Build per-strategy implemented route plans.
    if (choice == 1 || choice == 5)
        plan_opt = build_plan(impact, 1, off, active_opt, bad_opt);
    if (choice == 2 || choice == 5)
        plan_sup = build_plan(impact, 2, off, active_sup, bad_sup);
    if (choice == 3 || choice == 5)
        plan_cost = build_plan(impact, 3, off, active_cost, bad_cost);
    if (choice == 4 || choice == 5)
        plan_time = build_plan(impact, 4, off, active_time, bad_time);

    // Resolve final active/unrecoverable node view based on user choice.
    unordered_set<string> active_nodes;
    unordered_set<string> bad_nodes;

    if (choice == 1)
    {
        active_nodes = active_opt;
        bad_nodes = bad_opt;
    }
    else if (choice == 2)
    {
        active_nodes = active_sup;
        bad_nodes = bad_sup;
    }
    else if (choice == 3)
    {
        active_nodes = active_cost;
        bad_nodes = bad_cost;
    }
    else if (choice == 4)
    {
        active_nodes = active_time;
        bad_nodes = bad_time;
    }
    // For "Run All", mark node active if ANY strategy can recover it.
    else
    {
        for (const string &node : impact)
        {
            if (active_opt.count(node) || active_sup.count(node) || active_cost.count(node) || active_time.count(node))
                active_nodes.insert(node);
            else
                bad_nodes.insert(node);
        }
    }

    // Final offline set = hard failure nodes + unrecoverable impacted nodes.
    unordered_set<string> final_off = off;
    final_off.insert(bad_nodes.begin(), bad_nodes.end());

    // -----------------------------------------------------------------
    // Step 5: Render final state and persist strategy outputs.
    // -----------------------------------------------------------------
    cout << "[\033[32mSUCCESS\033[0m] SYSTEM STABILIZED.\n\n";
    cout << ">> Reactivated affected nodes: " << active_nodes.size() << "\n";
    cout << ">> Still inactive (unrecoverable) nodes: " << bad_nodes.size() << "\n\n";

    cout << "========================================================\n";
    cout << "        NETWORK STATE AFTER OPTIMIZATION/STABILIZATION        \n";
    cout << "========================================================\n\n";
    show_graph(final_off);

    if (choice == 1 || choice == 5)
    {
        vector<string> tmp(bad_opt.begin(), bad_opt.end());
        sort(tmp.begin(), tmp.end());
        show_result("MOST OPTIMAL", best_opt_route, "most_optimal", plan_opt, tmp);
    }
    if (choice == 2 || choice == 5)
    {
        vector<string> tmp(bad_sup.begin(), bad_sup.end());
        sort(tmp.begin(), tmp.end());
        show_result("MAX SUPPLY", max_supply_route, "max_supply", plan_sup, tmp);
    }
    if (choice == 3 || choice == 5)
    {
        vector<string> tmp(bad_cost.begin(), bad_cost.end());
        sort(tmp.begin(), tmp.end());
        show_result("MIN COST", min_cost_route, "min_cost", plan_cost, tmp);
    }
    if (choice == 4 || choice == 5)
    {
        vector<string> tmp(bad_time.begin(), bad_time.end());
        sort(tmp.begin(), tmp.end());
        show_result("MIN TIME", min_time_route, "min_time", plan_time, tmp);
    }

    cout << ">> Optimization logs saved individually to 'backend/suggestions/'\n";
    return 0;
}