#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <array>


struct Edge {
    size_t u = 0, v = 0;
    std::array<std::string, 2> weight; // [0]=hh, [1]=kz
    bool is_active = true;
};

class Graph {
private:
    size_t n = 0;
    std::vector<Edge> edges;
    std::vector<std::vector<size_t>> adj;
    std::string r = "1";
    int sign = 1;

public:
    explicit Graph(const std::string& file_path);
    explicit Graph(size_t n_vertices);

    void print() const;

    size_t add_edge(size_t u, size_t v, const std::string& w0, const std::string& w1);
    void remove_edge_by_id(size_t id) {edges[id].is_active = false; }
    void merge_vertices(size_t from, size_t to);

    size_t get_edge_count() const { return edges.size(); }
    size_t n_vertices() const { return n; }
    const Edge& get_edge(size_t id) const { return edges[id]; }
    const std::string& get_r() const { return r; }

    void flip_sign() { sign *= -1; }

    size_t active_degree(size_t x) const;

    bool same_pg_pair(const Edge& e1, const Edge& e2) const;
    bool is_pg_edge(const Edge& e) const;
    std::string pg_label(const Edge& e) const;
    void flip_pg_direction(Edge& e);

    void rebuild_adj();

    bool simplify_pg_degenerate_once();

    bool simplify_pg_pg_series_once();
    bool simplify_pg_pg_parallel_once();

    bool simplify_pg_series_once();
    bool simplify_pg_parallel_once();

    bool simplify_parallel_once();
    bool simplify_series_once();
    void simplify();

};


#endif // GRAPH_H