// dijkstra.cpp
// -----------------------------------------------------------------------------
// Simple multi-strategy route search for src1.
//
// STRATEGY MAP (mode)
// -------------------
// mode=1 -> Most Optimal (min composite score)
// mode=2 -> Max Supply (maximize bottleneck capacity)
// mode=3 -> Min Cost (minimize INR)
// mode=4 -> Min Time (minimize minutes)
//
// ALGORITHM IDEA
// --------------
// - mode 1/3/4 use Dijkstra-like min-heap shortest path.
// - mode 2 uses widest-path variant (max-heap), where path value is the
//   minimum edge capacity along the route, and we maximize that value.
//
// EDGE CASES HANDLED
// ------------------
// - Offline nodes are skipped via the "off" set.
// - Stale heap entries are ignored.
// - If destination has no parent, returns empty path.
//
// EXAMPLE (mode=3)
// ----------------
// Suppose A->B cost=50, A->C cost=20, C->B cost=10.
// Best cost A->B is A->C->B with total 30.
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>

using namespace std;

vector<string> dijkstra(const string &start, const string &end, int mode, const unordered_set<string> &off)
{
    // Min-heap for minimizing objective weight.
    priority_queue<pair<double, string>, vector<pair<double, string>>, greater<pair<double, string>>> min_heap;
    // Max-heap for maximizing bottleneck capacity (mode 2).
    priority_queue<pair<int, string>> max_heap;

    // dist[node] means:
    // - mode 1/3/4: best (lowest) total weight so far
    // - mode 2: best (highest) bottleneck capacity so far
    unordered_map<string, double> dist;
    // Parent pointer map for path reconstruction.
    unordered_map<string, string> prev;

    for (const auto &node : all_nodes)
        dist[node] = (mode == 2) ? -1 : 1e9;

    if (mode == 2)
    {
        // Start with effectively infinite capacity at source.
        max_heap.push({1e9, start});
        dist[start] = 1e9;
    }
    else
    {
        // Classic shortest-path initialization.
        min_heap.push({0.0, start});
        dist[start] = 0.0;
    }

    while (!min_heap.empty() || !max_heap.empty())
    {
        double cur = 0.0;
        string node;

        if (mode == 2)
        {
            cur = max_heap.top().first;
            node = max_heap.top().second;
            max_heap.pop();

            // Ignore stale entries that are worse than recorded best.
            if (cur < dist[node])
                continue;
        }
        else
        {
            cur = min_heap.top().first;
            node = min_heap.top().second;
            min_heap.pop();

            // Ignore stale entries that are worse than recorded best.
            if (cur > dist[node])
                continue;
        }

        // Optional early stop once destination is popped/settled.
        if (node == end)
            break;

        // Relax all outgoing edges.
        for (const auto &edge : graph[node])
        {
            // Do not route through offline nodes.
            if (off.count(edge.to))
                continue;

            if (mode == 2)
            {
                // Bottleneck of extended path = min(current bottleneck, edge cap).
                int path_cap = min((int)cur, edge.capacity);
                if (path_cap > dist[edge.to])
                {
                    dist[edge.to] = path_cap;
                    prev[edge.to] = node;
                    max_heap.push({path_cap, edge.to});
                }
            }
            else
            {
                // Objective-specific edge weight.
                double w = 0.0;
                if (mode == 1)
                    // Composite heuristic used by backend ranking.
                    w = max(1.0, edge.cost_inr + (edge.time_mins * 20.0) - (edge.capacity * 2.0));
                else if (mode == 3)
                    w = edge.cost_inr;
                else if (mode == 4)
                    w = edge.time_mins;

                // Standard relaxation step.
                if (cur + w < dist[edge.to])
                {
                    dist[edge.to] = cur + w;
                    prev[edge.to] = node;
                    min_heap.push({dist[edge.to], edge.to});
                }
            }
        }
    }

    vector<string> path;
    if (!prev.count(end))
        return path;

    // Rebuild reverse chain end -> ... -> start, then reverse.
    for (string at = end; at != ""; at = prev[at])
    {
        path.push_back(at);
        if (at == start)
            break;
    }

    reverse(path.begin(), path.end());
    return path;
}