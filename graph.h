#ifndef GRAPH_H
#define GRAPH_H

#include <cstddef>
#include <string>
#include <vector>
#include <array>

struct Edge {
    size_t u = 0, v = 0;
    std::array<long long, 2> weight = {0, 0}; // [0]= Y - conductivity, [1]= Z - resistance
    std::string pg_label;
    bool is_active = true;
    int edge_type = 0;
};

class Graph {
private:
    size_t n = 0;
    std::vector<Edge> edges;
    std::vector<std::vector<size_t>> adj;
    long long r = 1;
    int sign = 1;
    size_t max_recursion_depth = 0;

    long long solve_impl(size_t depth);

public:
    explicit Graph(const std::string& file_path);
    explicit Graph(size_t n_vertices);

    void print() const;
    void print_rezult() const;

    size_t add_edge(size_t u, size_t v, long long w0, long long w1);
    void remove_edge_by_id(size_t id) {edges[id].is_active = false; }
    void merge_vertices(size_t from, size_t to);

    size_t get_edge_count() const { return edges.size(); }
    size_t n_vertices() const { return n; }
    const Edge& get_edge(size_t id) const { return edges[id]; }
    long long get_r() const { return r; }
    size_t get_max_recursion_depth() const { return max_recursion_depth; }

    void flip_sign() { sign *= -1; }

    size_t active_degree(size_t x) const;

    bool same_pg_pair(const Edge& e1, const Edge& e2) const;
    bool is_pg_edge(const Edge& e) const;
    int pg_index(const Edge& e) const;
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

    bool simplify_leaf_once();
    bool simplify_loop_once();

    bool is_solved() const;
    size_t choose_edge_for_recursion() const;

    void neutralize_edge_by_id(size_t id);
    void contract_edge_by_id(size_t id);

    long long solve();

    void simplify();

};


#endif // GRAPH_H
