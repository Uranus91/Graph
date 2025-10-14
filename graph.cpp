// graph.cpp
#include "graph.h"
#include <iostream>
#include <queue>
#include <stdexcept>
#include <set>
#include <map>


Graph::Graph(int n) : n(n), adj(n), r(""), finished(false) {}
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

void Graph::simplify() {
    bool changed = true;
    while(changed) {
        changed = false;
        for (int i = 0; i < n; i++) {
            if (adj[i].empty())
                continue;
            std::multiset<std::pair<int, std::string>> neighbors;
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
                
                remove_edge(i, u, w1);
                remove_edge(i, w, w2);
                add_edge(u, w, w1 + "+" + w2);
                changed = true;
                break;
            }
        }
    }
}

void Graph::takeaway() {
    for (int u = 0; u < n; ++u) {
        for (const auto& edge : adj[u]) {
            if (u == edge.first) {
                r = (r != "") ? (r + "(" + edge.second + ")") : ("(" + edge.second + ")");
                remove_edge(u, u, edge.second);
            }
            if (edge.second == "")
                break;
        }       
    }
}

void Graph::paralel() {
    std::set<std::string> weights;
    for (int u = 0; u < n; ++u) {
        if (adj[u].empty())
            continue;
        for (const auto& edge : adj[u]) {
            weights.insert(edge.second);
        }
    }

    std::string result;
    
    for (const auto& skip : weights) {
        if (!result.empty()) result += "+";
        
        std::string product;
        for (const auto& i : weights) {
            if (i != skip) {
                if (!product.empty()) product += "";
                product += (i.length() >= 3) ? ("(" + i + ")") : (i);
            }
        }
        result += product;
    }
    
    r = r + ((r == "") ? ("(" + result + ")") : (" (" + result + ")")) ;
    finished = true;
}

void Graph::print() const {
    std::cout << r << std::endl;
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

int Graph::vertex_count() const {
    int count = 0;
    for (int i = 0; i < n; ++i) {
        if (!adj[i].empty()) {
            count++;
        }
    }
    return count;
}

int Graph::edges_count() const {
    std::set<std::pair<int, int>> counter;
    for (int i = 0; i < n; i++) {
        for(const auto& edge : adj[i]) {
            int j = edge.first;
            if (i < j) {
                counter.insert({i, j});
            }
        }
    }
    return counter.size();
}

std::vector<std::vector<std::string>> Graph::split_by_edges() const {
    std::map<std::pair<int, int>, std::vector<std::string>> edge_map;

    for (int u = 0; u < n; ++u) {
        for (const auto& edge : adj[u]) {
            int v = edge.first;
            const std::string& w = edge.second;

            // Нормализуем пару: меньший индекс — первый
            if (u <= v) {
                edge_map[{u, v}].push_back(w);
            }
            // Если u > v — пропускаем, чтобы не дублировать
        }
    }

    // Извлекаем только векторы весов
    std::vector<std::vector<std::string>> result;
    result.reserve(edge_map.size());
    for (const auto& kv : edge_map) {
        result.push_back(kv.second);
    }

    return result;
}

std::string Graph::recursive_algorithm() {
    if (finished) {
        return "";
    }
    simplify();
    takeaway();

    if (edges_count() == 0) {
        finished = true;
        return r.empty() ? "1" : r;
    }
    if (vertex_count() == 2) {
        paralel(); // это установит finished = true и заполнит r
        finished = true;
        return r;
    }
    // Найти любое ребро
    int u = -1, v = -1;
    std::string weight;

    for (int i = 0; i < n; ++i) {
        if (adj[i].empty()) continue;

        for (const auto& edge : adj[i]) {
            u = i;
            v = edge.first;
            weight = edge.second;
            break;
        }
        if (u != -1) break;
    }

    if (u == -1) {
        finished = true;  // нет рёбер
        return "";
    }

    // Создаём 2 копии
    Graph gr1 = *this;
    gr1.remove_edge(u, v, weight);
 

    Graph gr2 = gr1;
    gr1.r = gr1.r + ((r == "" || r.size() <= 3) ? (weight ) : (" (" + weight + ")")); 
    gr2.merge_vertex(u, v, weight);

    gr1.simplify();
    gr1.takeaway();
    if (gr1.vertex_count() == 2) {
        gr1.paralel();
    }
    gr2.simplify();
    gr2.takeaway();
    if (gr2.vertex_count() == 2) {
        gr2.paralel();
    }

    // Рекурсивно вызываем для каждого
    std::string result1 = gr1.recursive_algorithm();
    std::string result2 = gr2.recursive_algorithm();

    finished = true;

    // Если результаты пустые, используем r
    if (result1.empty()) result1 = gr1.r;
    if (result2.empty()) result2 = gr2.r;

    return "(" + result1 + ") + (" + result2 + ")";
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