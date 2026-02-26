#include "graph.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

Graph::Graph(const std::string& file_path) {
    std::ifstream in(file_path);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    size_t m, n;
    in >> n >> m;
    this->n = n;
    edges.clear();
    adj.assign(this->n,{});
    edges.reserve(m);

    for (size_t i = 0; i < m; i++) {
    std::string w;
    size_t u = 0, v = 0;
    in >> w >> u >> v;

    Dir dir = Dir::Undirected;
    if (w == "P" || w == "G") dir = Dir::UtoV; // считаем, что запись u v означает u->v

    add_edge(u - 1, v - 1, std::move(w), dir);
}
}

Graph::Graph(size_t n_vertices) : n(n_vertices), adj(n_vertices) {}

void Graph::append_to_multiplier(const std::string& w) {
    if (w.empty()) return;

    if (w == "0") {
        r = "0";
        return;
    }

    if (r == "0") return;

    if (w == "-1") {
        if (r.empty()) { r = "-1"; return; }
        if (r[0] == '-') r.erase(r.begin());   // "-A" * (-1) = "A"
        else r.insert(r.begin(), '-');         // "A" * (-1) = "-A"
        return;
    }

    if (!r.empty()) r.push_back('*');
    r.append(w);
}

static size_t from_node(const Edge& e) {
    // Для PG считаем, что направление задано через e.dir
    if (e.dir == Dir::VtoU) return e.v;
    return e.u; // UtoV или Undirected (на Undirected лучше не полагаться)
}

static size_t to_node(const Edge& e) {
    if (e.dir == Dir::VtoU) return e.u;
    return e.v;
}


void Graph::print() const {
    std::cout << "Graph\n";
    std::cout << "Vertices: " << n << "\n";
    std::cout << "Edges:    " << edges.size() << "\n";
    std::cout << "Multiplier r: " << r << "\n";

    for (size_t id = 0; id < edges.size(); ++id) {
        const auto& e = edges[id];
        std::cout << id << ": " << (e.u + 1) << " " << (e.v + 1)
                  << " " << e.weight;

        if (e.weight == "P" || e.weight == "G") {
            if (e.dir == Dir::UtoV) std::cout << " dir: u->v";
            else if (e.dir == Dir::VtoU) std::cout << " dir: v->u";
            else std::cout << " dir: undirected";
        }

        std::cout << " " << (e.is_active ? "active" : "deleted") << "\n";
    }
}

size_t Graph::add_edge(size_t u, size_t v, std::string weight, Dir dir) {
    size_t id = edges.size();
    edges.push_back(Edge{u, v, std::move(weight), true, dir});

    adj[u].push_back(id);
    if (v != u) adj[v].push_back(id);
    return id;
}

void Graph::remove_edge(size_t u, size_t v, std::string weight) {
    for (auto& e : edges) {
        if (!e.is_active) continue;
        const bool same_uv = (e.u == u && e.v == v) || (e.u == v && e.v == u);
        if (same_uv && e.weight == weight) {
            e.is_active = false;
            return;
        }
    }
}

void Graph::contract_edge_by_id(std::size_t id) {
    if (id >= edges.size()) return;
    if (!edges[id].is_active) return;

    size_t u = edges[id].u;
    size_t v = edges[id].v;

    // петля: просто отключаем
    if (u == v) {
        edges[id].is_active = false;
        return;
    }

    size_t keep = u;
    size_t kill = v;

    // 1) отключаем само ребро P/G (оно схлопнулось)
    edges[id].is_active = false;

    // 2) перенаправляем все активные ребра, инцидентные kill, на keep
    for (size_t eid : adj[kill]) {
        if (!edges[eid].is_active) continue;
        if (eid == id) continue; // уже отключили

        auto& e = edges[eid];
        if (e.u == kill) e.u = keep;
        if (e.v == kill) e.v = keep;

        adj[keep].push_back(eid);

        // добавить eid в adj[keep], чтобы keep видел это ребро
    }

    // 3) очищаем список смежности “убитой” вершины
    adj[kill].clear();
}

bool Graph::is_pg_edge(size_t id) const {
    const auto& w = edges[id].weight;
    return (w == "G" || w == "P");
}

std::vector<size_t> Graph::active_incident_edges(size_t vtx) const {
    std::vector<size_t> res;
    for (size_t id : adj[vtx]) {
        if (edges[id].is_active) res.push_back(id);
    }
    return res;
}

size_t Graph::active_degree(size_t v) const {
    size_t deg = 0;
    for (size_t id : adj[v]) {
        if (edges[id].is_active) ++deg;
    }
    return deg;
}

bool Graph::check_degenerate_pg() {
    if (r == "0") return true; // уже вырождено

    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (!is_pg_edge(id)) continue;

        const auto& e = edges[id];

        // петля
        if (e.u == e.v) {
            r = "0";
            return true;
        }

        // лист
        if (active_degree(e.u) == 1 || active_degree(e.v) == 1) {
            r = "0";
            return true;
        }
    }

    return false;
}

bool Graph::simplify_parallel_pg_once() {
    // key = (min(u,v), max(u,v))
    std::map<std::pair<size_t, size_t>, std::vector<size_t>> groups;

    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        auto u = edges[id].u;
        auto v = edges[id].v;
        if (u == v) continue; // петли можно отдельно обрабатывать
        if (u > v) std::swap(u, v);
        groups[{u, v}].push_back(id);
    }

    bool changed = false;

    for (auto& kv : groups) {
        auto& ids = kv.second;
        if (ids.size() < 2) continue;

        bool has_pg = false;
        for (size_t id : ids) {
            if (is_pg_edge(id)) { has_pg = true; break; }
        }
        if (!has_pg) continue;

        // Есть P/G — значит все НЕ P/G параллельные выносим в множитель и удаляем
        for (size_t id : ids) {
            if (!edges[id].is_active) continue;
            if (is_pg_edge(id)) continue;

            append_to_multiplier(edges[id].weight);
            edges[id].is_active = false;
            changed = true;
        }
    }

    return changed;
}

bool Graph::simplify_pg_series_once() {
    for (size_t x = 0; x < n; ++x) {
        auto inc = active_incident_edges(x);
        if (inc.size() != 2) continue;

        size_t e1 = inc[0];
        size_t e2 = inc[1];

        const bool e1_pg = is_pg_edge(e1);
        const bool e2_pg = is_pg_edge(e2);

        if (e1_pg && e2_pg) {
            const Edge& a = edges[e1];
            const Edge& b = edges[e2];

            size_t a_from = from_node(a), a_to = to_node(a);
            size_t b_from = from_node(b), b_to = to_node(b);

            // "в одну сторону" вдоль цепочки через x
            bool same_chain_dir =
                (a_to == x && b_from == x) ||
                (b_to == x && a_from == x);

            if (same_chain_dir) {
                append_to_multiplier("-1");
            }

            edges[e1].is_active = false;
            edges[e2].is_active = false;
            return true;
        }

        if (e1_pg && !e2_pg) {
            contract_edge_by_id(e2); // стягиваем обычное
            return true;
        }
        if (e2_pg && !e1_pg) {
            contract_edge_by_id(e1); // стягиваем обычное
            return true;
        }
        if (e1_pg && e2_pg) {
            contract_edge_by_id(e1);
            return true;
        }
    }
    return false;
}

void Graph::simplify() {
    bool changed = true;
    while (changed) {
        if (check_degenerate_pg()) return;

        changed = false;
        if (simplify_parallel_pg_once()) changed = true;
        if (simplify_pg_series_once()) changed = true;

    }
    finalize_pg_cycle_end();
}

bool Graph::finalize_pg_cycle_end() {
    // соберём все активные ребра
    std::vector<size_t> active;
    active.reserve(edges.size());
    for (size_t id = 0; id < edges.size(); ++id) {
        if (edges[id].is_active) active.push_back(id);
    }

    // должен остаться ровно контур из двух ребер P и G
    if (active.size() != 2) return false;

    size_t id1 = active[0];
    size_t id2 = active[1];

    const Edge& e1 = edges[id1];
    const Edge& e2 = edges[id2];

    // оба должны быть PG
    if (!(is_pg_edge(id1) && is_pg_edge(id2))) return false;

    // и один должен быть P, другой G (если хочешь строго)
    bool onePoneG =
        (e1.weight == "P" && e2.weight == "G") ||
        (e1.weight == "G" && e2.weight == "P");
    if (!onePoneG) return false;

    // направления
    auto from = [&](const Edge& e) -> size_t {
        return (e.dir == Dir::VtoU) ? e.v : e.u; // UtoV
    };
    auto to = [&](const Edge& e) -> size_t {
        return (e.dir == Dir::VtoU) ? e.u : e.v;
    };

    size_t f1 = from(e1), t1 = to(e1);
    size_t f2 = from(e2), t2 = to(e2);

    // “навстречу” = противоположные направления: f1==t2 && t1==f2  -> знак +
    // “в одну сторону” = одинаково: f1==f2 && t1==t2 -> знак -
    if (f1 == t2 && t1 == f2) {
        if (r.empty()) r = "1";
        edges[id1].is_active = false;
        edges[id2].is_active = false;
        return true;
    }

    if (f1 == f2 && t1 == t2) {
        // знак - : домножаем на -1 и удаляем оба PG
        append_to_multiplier("-1");
        edges[id1].is_active = false;
        edges[id2].is_active = false;
        return true;
    }

    // если вдруг остались два PG, но они не образуют ожидаемый контур — не трогаем
    return false;
}