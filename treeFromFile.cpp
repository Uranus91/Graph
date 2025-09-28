#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <set>
#include <fstream>
#include <string>

class Graph {
private:
    std::vector<std::vector<std::pair<int, std::string>>> adj; // adj[u] = список пар (v, вес)
    int n; // количество вершин

public:
    // Конструктор: создаёт граф с n вершинами
    Graph(int n) : n(n), adj(n) {}
    // Добавить ребро между u и v с весом w
    void add_edge(int u, int v, std::string w) {
        if (u < 0 || u >= n || v < 0 || v >= n) return;
        adj[u].push_back({v, w});
        adj[v].push_back({u, w}); // неориентированный граф
    }

    void remove_edge(int u, int v, std::string w) {
        if (u < 0 || u >= n || v < 0 || v >= n) return;

        // Удалить ребро из u в v (с весом w)
        for (auto it = adj[u].begin(); it != adj[u].end(); ++it) {
            if (it->first == v && it->second == w) {
                adj[u].erase(it);
                break; // удалили первое найденное ребро
            }
        }
        // Удалить ребро из v в u (с весом w)
        for (auto it = adj[v].begin(); it != adj[v].end(); ++it) {
            if (it->first == u && it->second == w) {
                adj[v].erase(it);
                break; // удалили первое найденное ребро
            }
        }
    }

    void print() const {
        for (int u = 0; u < n; ++u) {
            std::cout << "Verticle " << u + 1 << ": ";
            for (const auto& edge : adj[u]) {
                std::cout << "(" << edge.first + 1 << ", " << edge.second << ") ";
            }
            std::cout << "\n";
        }
    }

    bool is_connected() const {
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
        return n == count;    
    }

    struct TreeResult;  
    TreeResult make_tree() const;
};


struct Graph::TreeResult {
    Graph tree;
    std::vector<std::vector<std::pair<int, int>>> hords;

    int total_removed_edges() const {
        int total = 0;
        for (const auto& vec : hords) {
            total += vec.size();
        }
        return total / 2;
    }

    void print() const {
        std::cout << std::endl << "Removed " << total_removed_edges()  << " edges:" << std::endl;
        for (int u = 0; u < hords.size(); ++u) {
            std::cout << "Verticle " << u + 1 << ": ";
            for (const auto& edge : hords[u]) {
                std::cout << "(" << edge.first + 1 << "," << edge.second << ") ";
            }
            std::cout << "\n";
        }
        std::cout << std::endl << "Tree from graph is:" << std::endl;
        tree.print();
    }
};

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
                parent[v] = u;           // ← запомнили родителя
                q.push(v);
            } else if (v != parent[u]) { // ← не добавляем обратное ребро дерева!
                hords[u].emplace_back(v, edge.second);
            }
        }
    }

    return {tree, hords};
}


int main() {

    std::ifstream inFile("input.txt");

    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file 'input.txt'." << std::endl;
        return 1;
    }

    int ver, edg;
    inFile >> ver >> edg;

    Graph gr(ver);
    int u, v;
    std::string weight;
    while (inFile >> u >> v >> weight) 
        gr.add_edge(u - 1, v - 1, weight);

    std::cout << std::endl;
    gr.print();
    if (!gr.is_connected()) {
        std::cout << "Not connected";
        return 0;
    }

    Graph::TreeResult tree = gr.make_tree();
    tree.print();
    
    return 0;
}

