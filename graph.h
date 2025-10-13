// graph.h
#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>

class Graph {
private:
    std::vector<std::vector<std::pair<int, std::string>>> adj;
    int n;

public:
    std::string r;
    bool finished;
    Graph(int n);

    void add_edge(int u, int v, std::string w);
    void remove_edge(int u, int v, std::string w);
    void merge_vertex(int u, int v, std::string w);
    void simplify();
    void takeaway();
    void paralel();

    std::string recursive_algorithm();

    void print() const;
    int vertex_count() const;
    int edges_count() const;
    bool is_connected() const;
    struct TreeResult;  // ← forward declaration

    TreeResult make_tree() const;
};

// Теперь объявляем TreeResult ПОСЛЕ полного определения Graph
struct Graph::TreeResult {
    Graph tree;
    std::vector<std::vector<std::pair<int, std::string>>> hords;

    int total_removed_edges() const;
    void print() const;
};

#endif // GRAPH_H