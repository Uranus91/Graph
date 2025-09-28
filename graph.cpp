// graph.cpp
#include "graph.h"
#include <iostream>
#include <queue>
#include <stdexcept>

Graph::Graph(int n) : n(n), adj(n) {}
// Реализация методов Graph
void Graph::add_edge(int u, int v, std::string w) {
    if (u < 0 || u >= n || v < 0 || v >= n) return;
    adj[u].push_back({v, w});
    adj[v].push_back({u, w});
}

void Graph::remove_edge(int u, int v, std::string w) {
    if (u < 0 || u >= n || v < 0 || v >= n) return;

    for (auto it = adj[u].begin(); it != adj[u].end(); ++it) {
        if (it->first == v && it->second == w) {
            adj[u].erase(it);
            break;
        }
    }
    for (auto it = adj[v].begin(); it != adj[v].end(); ++it) {
        if (it->first == u && it->second == w) {
            adj[v].erase(it);
            break;
        }
    }
}

void Graph::print() const {
    for (int u = 0; u < n; ++u) {
        std::cout << "Vertex " << u + 1 << ": ";
        for (const auto& edge : adj[u]) {
            std::cout << "(" << edge.first + 1 << ", " << edge.second << ") ";
        }
        std::cout << "\n";
    }
}

bool Graph::is_connected() const {
    std::vector<bool> visited(n, false);
    std::queue<int> q;
    q.push(0);
    visited[0] = true;
    int count = 1;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& edge : adj[u]) {
            int v = edge.first;
            if (!visited[v]) {
                visited[v] = true;
                count++;
                q.push(v);
            }
        }
    }
    return count == n;
}

// Реализация TreeResult
int Graph::TreeResult::total_removed_edges() const {
    int total = 0;
    for (const auto& vec : hords) {
        total += vec.size();
    }
    return total / 2;
}

void Graph::TreeResult::print() const {
    std::cout << std::endl << "Removed " << total_removed_edges() << " edges:" << std::endl;
    for (int u = 0; u < hords.size(); ++u) {
        std::cout << "Vertex " << u + 1 << ": ";
        for (const auto& edge : hords[u]) {
            std::cout << "(" << edge.first + 1 << "," << edge.second << ") ";
        }
        std::cout << "\n";
    }
    std::cout << std::endl << "Tree from graph is:" << std::endl;
    tree.print();
}

// make_tree
Graph::TreeResult Graph::make_tree() const {
    if (!is_connected()) {
        throw std::runtime_error("Graph is not connected!");
    }

    std::vector<bool> visited(n, false);
    std::vector<int> parent(n, -1);
    Graph tree(n);
    std::vector<std::vector<std::pair<int, std::string>>> hords(n);
    std::queue<int> q;

    q.push(0);
    visited[0] = true;

    while (!q.empty()) {
        int u = q.front();
        q.pop();

        for (const auto& edge : adj[u]) {
            int v = edge.first;
            if (!visited[v]) {
                tree.add_edge(u, v, edge.second);
                visited[v] = true;
                parent[v] = u;
                q.push(v);
            } else if (v != parent[u]) {
                hords[u].emplace_back(v, edge.second);
            }
        }
    }

    return {tree, hords};
}