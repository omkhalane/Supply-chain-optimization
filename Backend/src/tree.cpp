// tree.cpp
// -----------------------------------------------------------------------------
// BFS impacted-tree utilities for src1.
//
// WHAT THIS MODULE DOES
// ---------------------
// After a failure at node F, this module discovers all downstream nodes
// reachable from F using BFS and builds a general n-child tree representation.
//
// WHY BFS
// -------
// BFS explores level by level:
// - Level 0: failed node itself
// - Level 1: direct dependents
// - Level 2: dependents of dependents
// ...
// This gives a clear operational view of disruption waves.
//
// EXAMPLE
// If graph edges are A->B, A->C, B->D and failure is A:
// - Levels become: [A], [B,C], [D]
// - Impacted set becomes: {B,C,D}
// -----------------------------------------------------------------------------

#include <bits/stdc++.h>

using namespace std;

struct Tree
{
    // Failed node that starts the impact wave.
    string root;
    // Parent -> direct BFS children.
    unordered_map<string, vector<string>> child;
    // All impacted nodes excluding root.
    unordered_set<string> impact;
    // Level-order buckets for display/debug.
    vector<vector<string>> level;
};

// BFS()
// -----
// Builds impacted tree from failure node.
//
// IMPLEMENTATION NOTES
// - "seen" avoids revisiting nodes (important for cyclic graphs).
// - First discovery fixes each node's parent in the tree.
// - The function uses global adjacency list: graph[node].
Tree BFS(const string &fail)
{
    Tree tree;
    tree.root = fail;

    unordered_set<string> seen;
    queue<pair<string, int>> q;

    seen.insert(fail);
    q.push({fail, 0});

    while (!q.empty())
    {
        string node = q.front().first;
        int depth = q.front().second;
        q.pop();

        if (tree.level.size() <= (size_t)depth)
            tree.level.push_back({});
        tree.level[depth].push_back(node);

        // Expand all outgoing dependencies from current node.
        for (const auto &edge : graph[node])
        {
            if (seen.count(edge.to))
                continue;

            seen.insert(edge.to);
            tree.child[node].push_back(edge.to);
            tree.impact.insert(edge.to);
            q.push({edge.to, depth + 1});
        }
    }

    return tree;
}

// tree_display()
// --------------
// Prints a readable diagnostics view of the impacted tree:
// 1) Root + impacted count
// 2) BFS levels
// 3) Parent -> children edges (sorted for stable output)
//
// EXAMPLE SNIPPET (conceptual)
// Root Failure Node: A
// Impacted Nodes (excluding root): 3
// BFS Levels:
//   Level 0: A
//   Level 1: B, C
//   Level 2: D
void tree_display(const Tree &tree)
{
    cout << "========================================================\n";
    cout << "         IMPACTED GENERAL TREE (BFS FROM FAILURE)       \n";
    cout << "========================================================\n\n";

    if (tree.root.empty())
    {
        cout << "[!] No failure root provided.\n\n";
        return;
    }

    cout << "Root Failure Node: " << tree.root << "\n";
    cout << "Impacted Nodes (excluding root): " << tree.impact.size() << "\n\n";

    cout << "BFS Levels:\n";
    for (size_t i = 0; i < tree.level.size(); i++)
    {
        cout << "  Level " << i << ": ";
        for (size_t j = 0; j < tree.level[i].size(); j++)
        {
            if (j)
                cout << ", ";
            cout << tree.level[i][j];
        }
        cout << "\n";
    }

    cout << "\nImpacted Tree Edges (Parent -> Children):\n";

    map<string, vector<string>> ordered;
    for (const auto &item : tree.child)
    {
        vector<string> kids = item.second;
        sort(kids.begin(), kids.end());
        ordered[item.first] = kids;
    }

    if (ordered.empty())
    {
        cout << "  (No downstream impacted nodes)\n\n";
        return;
    }

    for (const auto &item : ordered)
    {
        cout << "  " << item.first << " -> ";
        for (size_t i = 0; i < item.second.size(); i++)
        {
            if (i)
                cout << ", ";
            cout << item.second[i];
        }
        cout << "\n";
    }

    cout << "\n";
}

// Convenience helper when only impacted node set is needed.
unordered_set<string> get_impact(const string &fail)
{
    return BFS(fail).impact;
}