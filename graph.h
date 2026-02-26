#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <limits>

enum class Dir : unsigned char { Undirected, UtoV, VtoU };

struct Edge {
    size_t u = 0;
    size_t v = 0;
    std::string weight;
    bool is_active = true;
    Dir dir = Dir::Undirected;
};

class Graph {
private:
    size_t n = 0;
    std::vector<Edge> edges;

    std::vector<std::vector<std::size_t>> adj;

    std::string r;
    

    bool is_edge_active(size_t id) const { return edges[id].is_active; }
    bool is_pg_edge(size_t id) const; 

    bool check_degenerate_pg();

    void append_to_multiplier(const std::string& w);
    void contract_edge_by_id(size_t id);
    bool simplify_parallel_pg_once();
    bool simplify_pg_series_once();
    bool finalize_pg_cycle_end();

    std::vector<size_t> active_incident_edges(size_t vtx) const;
    size_t active_degree(std::size_t v) const;

public:
    explicit Graph(const std::string& file_path);
    explicit Graph(size_t n_vertices);

    void print() const;

    size_t get_edge_count() const { return edges.size(); }
    const std::string& get_multiplier() const { return r; }
    size_t n_vertices() const { return n; }
    const Edge& get_edge(size_t id) const { return edges[id]; }


    size_t add_edge(size_t u, size_t v, std::string weight, Dir dir = Dir::Undirected);
    void remove_edge(size_t u, size_t v, std::string weight);

    void remove_edge_by_id(size_t id) { edges[id].is_active = false; }
    void restore_edge_by_id(size_t id) { edges[id].is_active = true; }

 // один проход 
    void simplify();

};


#endif // GRAPH_H