// graph.cpp
#include "graph.h"
#include <iostream>
#include <queue>
#include <stdexcept>
#include <set>

Graph::Graph(int n) : n(n), adj(n), r("1") {}
// Реализация методов Graph
void Graph::add_edge(int u, int v, std::string w) {
    if (u < 0 || u >= n || v < 0 || v >= n) return;
    adj[u].push_back({v, w});
    adj[v].push_back({u, w});
}

void Graph::remove_edge(int u, int v, std::string w) {
    if (u < 0 || u >= n || v < 0 || v >= n) return;

    for (auto it = adj[u].begin(); it != adj[u].end(); ) {
        if (it->first == v && it->second == w) {
            it = adj[u].erase(it);  
            break;
        } else {
            ++it;
        }
    }
    for (auto it = adj[v].begin(); it != adj[v].end(); ) {
        if (it->first == u && it->second == w) {
            it = adj[v].erase(it);
            break;
        } else {
            ++it;
        }
    }
}

void Graph::merge_vertex(int u, int v, std::string weight) {
    if (u < 0 || u >= n || v < 0 || v >= n) return;
    if (u == v) return;
    
    // Удаляем ребро u --weight-- v
    remove_edge(u, v, weight);
    
    // Создаем временную копию списка смежности v
    std::vector<std::pair<int, std::string>> v_edges = adj[v];
    
    // Удаляем все связи с вершиной v из соседних вершин
    for (const auto& edge : v_edges) {
        int neighbor = edge.first;
        std::string w = edge.second;
        
        // Удаляем ребро neighbor -> v
        for (auto it = adj[neighbor].begin(); it != adj[neighbor].end(); ) {
            if (it->first == v) {
                it = adj[neighbor].erase(it);
                break;
            } else {
                ++it;
            }
        }
        
        // Добавляем ребро neighbor -> u (если neighbor не u)
        if (neighbor != u) {
            add_edge(u, neighbor, w);
        }
    }
    
    // Очищаем список смежности v
    adj[v].clear();
}

int Graph::degree(int v) const {
    if (v < 0 || v >= n) return -1;
    std::set<int> unique_neighbors;
    for (const auto& edge : adj[v]) {
        unique_neighbors.insert(edge.first);
    }
    return unique_neighbors.size();
}

void Graph::simplify() {
    bool changed = true;
    while(changed) {
        changed = false;
        for (int i = 0; i < n; i++) {
            if (adj[i].empty())
                continue;
            std::set<std::pair<int, std::string>> neighbors;
            for(const auto& edge : adj[i]) {
                neighbors.insert({edge.first, edge.second});
            }
            int degr = neighbors.size();

            if (degr == 1) {
                std::pair<int, std::string> first_edge = *neighbors.begin();
                remove_edge(i,first_edge.first,first_edge.second);
                changed = true;
                break;
            }

            if (degr == 2) {
                auto it = neighbors.begin();
                auto [u, w1] = *it++;
                auto [w, w2] = *it;
                if (u != w) {
                    remove_edge(i, u, w1);
                    remove_edge(i, w, w2);
                    add_edge(u, w, w1 + " + " + w2);
                    changed = true;
                    break;
                }
            }
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