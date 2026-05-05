#include "graph.h"

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <utility>
#include <vector>
#include <map>

void Graph::print() const {
    std::cout << "Graph\n";
    std::cout << "Vertices: " << n << "\n";
    std::cout << "Edges:    " << edges.size() << "\n";

    for (size_t id = 0; id < edges.size(); ++id) {
        const auto& e = edges[id];
        std::cout << id + 1 << ": "
                  << (e.u + 1) << " " << (e.v + 1)
                  << "  Y = " << e.weight[0]
                  << "  Z = " << e.weight[1]
                  << (e.is_active ? "   active" : "   disactivated") <<   "\n";
    }
    std::cout << "Sign: " << sign << std::endl;
    std::cout << "Multiplier: " << this->r << std::endl;
}

void Graph::print_rezult() const {
    std::cout << "Graph\n";
    std::cout << "Sign: " << sign << std::endl;
    std::cout << "Multiplier: " << this->r << std::endl;
}

static bool needs_parens(const std::string& s) {
    return s.find('+') != std::string::npos;
}

static std::string parens_if_needed(const std::string& s) {
    if (s.empty()) return s;
    return needs_parens(s) ? ("(" + s + ")") : s;
}

static std::string add_expr(const std::string& a, const std::string& b) {
    if (a.empty() || a == "0") return b;
    if (b.empty() || b == "0") return a;
    return a + "+" + b;
}

static std::string mul_expr(const std::string& a, const std::string& b) {
    if (a.empty() || a == "1") return b;
    if (b.empty() || b == "1") return a;

    if (a == "0" || b == "0") return "0";

    return parens_if_needed(a) + "*" + parens_if_needed(b);
}

static std::string calc_1(const std::vector<std::string>& w) {
    std::string p;
    for (const auto& x : w) p = mul_expr(p, x);
    if (p.empty()) p = "1";
    return p;
}

static std::string calc_0(const std::vector<std::string>& par) {
    size_t k = par.size();
    if (k == 0) return "0";

    if (k == 1) return "1";

    std::vector<std::string> pref(k + 1, "1");
    std::vector<std::string> suf(k + 1, "1");

    for (size_t i = 0; i < k; ++i)
        pref[i + 1] = mul_expr(pref[i], par[i]);

    for (size_t i = k; i-- > 0; )
        suf[i] = mul_expr(par[i], suf[i + 1]);

    std::string sum;
    for (size_t i = 0; i < k; ++i) {
        std::string term = mul_expr(pref[i], suf[i + 1]);
        sum = add_expr(sum, term);
    }
    if (sum.empty()) sum = "0";
    return sum;
}

Graph::Graph(const std::string& file_path) {
    std::ifstream in(file_path);
    if (!in) throw std::runtime_error("Cannot open file: " + file_path);

    size_t m = 0, nn = 0;
    in >> nn >> m;
    if (!in) throw std::runtime_error("Bad header read");

    n = nn;
    edges.clear();
    adj.assign(n, {});

    // для обычных рёбер
    std::map<std::pair<size_t, size_t>, size_t> id_by_pair;
    std::map<std::pair<size_t, size_t>, size_t> parallel_idx_by_pair;
    std::vector<std::vector<std::string>> parallel;

    for (size_t i = 0; i < m; ++i) {
        std::string w;
        size_t u = 0, v = 0;
        in >> w >> u >> v;
        if (!in) {
            throw std::runtime_error("Bad edge read at line " + std::to_string(i + 2));
        }

        --u;
        --v;

        size_t a = (u < v) ? u : v;
        size_t b = (u < v) ? v : u;

        if (!w.empty() && (w[0] == 'P' || w[0] == 'G')) {
            Edge e;
            e.u = a;
            e.v = b;
            e.is_active = true;

            int idx = std::stoi(w.substr(1));
            bool forward = (u == a);
            std::string label = w + (forward ? ">" : "<");

            if (w[0] == 'P') {
                e.edge_type = idx;
                e.weight[0] = label;
                e.weight[1] = "0";
            } else {
                e.edge_type = -idx;
                e.weight[0] = "0";
                e.weight[1] = label;
            }

            size_t id = edges.size();
            edges.push_back(std::move(e));

            adj[a].push_back(id);
            if (b != a)
                adj[b].push_back(id);

            continue;
        }

        auto key = std::make_pair(a, b);
        auto it = id_by_pair.find(key);

        if (it == id_by_pair.end()) {
            Edge e;
            e.u = a;
            e.v = b;
            e.weight[0].clear();
            e.weight[1].clear();
            e.is_active = true;

            size_t id = edges.size();
            edges.push_back(std::move(e));

            id_by_pair[key] = id;
            parallel_idx_by_pair[key] = parallel.size();
            parallel.push_back({w});

            adj[a].push_back(id);
            if (b != a)
                adj[b].push_back(id);

        } else {
            size_t pidx = parallel_idx_by_pair[key];
            parallel[pidx].push_back(w);
        }
    }

    // считаем итоговые веса только для обычных рёбер
    for (const auto& kv : id_by_pair) {
        const auto& key = kv.first;
        size_t edge_id = kv.second;
        size_t pidx = parallel_idx_by_pair[key];

        edges[edge_id].weight[1] = calc_1(parallel[pidx]);
        edges[edge_id].weight[0] = calc_0(parallel[pidx]);
    }
}

Graph::Graph(size_t n_vertices) : n(n_vertices), adj(n_vertices) {}

size_t Graph::choose_edge_for_recursion() const {
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;

        // P/G пока не выносим рекурсивно
        if (is_pg_edge(edges[id])) continue;

        return id;
    }

    return static_cast<size_t>(-1);
}

size_t Graph::active_degree(size_t x) const {
    size_t deg = 0;
    for (size_t id : adj[x]) {
        if (edges[id].is_active) ++deg;
    }
    return deg;
}

bool Graph::is_solved() const {
    for (const auto& e : edges) {
        if (e.is_active) {
            return false;
        }
    }

    return true;
}

bool Graph::is_pg_edge(const Edge& e) const {
    return e.edge_type != 0;
}

int Graph::pg_index(const Edge& e) const {
    return (e.edge_type >= 0) ? e.edge_type : -e.edge_type;
}

bool Graph::same_pg_pair(const Edge& e1, const Edge& e2) const {
    if (!is_pg_edge(e1) || !is_pg_edge(e2)) return false;

    // одно должно быть P, другое G
    if ((e1.edge_type > 0 && e2.edge_type > 0) ||
        (e1.edge_type < 0 && e2.edge_type < 0)) {
        return false;
    }

    return pg_index(e1) == pg_index(e2);
}

std::string Graph::pg_label(const Edge& e) const {
    if (!e.weight[0].empty() && (e.weight[0][0] == 'P' || e.weight[0][0] == 'G'))
        return e.weight[0];

    if (!e.weight[1].empty() && (e.weight[1][0] == 'P' || e.weight[1][0] == 'G'))
        return e.weight[1];

    return "";
}

void Graph::flip_pg_direction(Edge& e) {
    if (!is_pg_edge(e)) return;

    std::string* s = nullptr;

    if (!e.weight[0].empty() && (e.weight[0][0] == 'P' || e.weight[0][0] == 'G'))
        s = &e.weight[0];
    else if (!e.weight[1].empty() && (e.weight[1][0] == 'P' || e.weight[1][0] == 'G'))
        s = &e.weight[1];

    if (!s || s->empty()) return;

    if (s->back() == '>')
        s->back() = '<';
    else if (s->back() == '<')
        s->back() = '>';
}

size_t Graph::add_edge(size_t u, size_t v, const std::string& w0, const std::string& w1)
{
    size_t a = (u < v) ? u : v;
    size_t b = (u < v) ? v : u;

    Edge e;
    e.u = a;
    e.v = b;
    e.weight[0] = w0;
    e.weight[1] = w1;
    e.is_active = true;

    size_t id = edges.size();
    edges.push_back(std::move(e));

    adj[a].push_back(id);
    if (b != a)
        adj[b].push_back(id);

    return id;
}

void Graph::merge_vertices(size_t from, size_t to) {
    if (from == to) return;

    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;

        if (edges[id].u == from) edges[id].u = to;
        if (edges[id].v == from) edges[id].v = to;

        if (edges[id].u > edges[id].v) {
            if (is_pg_edge(edges[id])) {
                flip_pg_direction(edges[id]);
            }
        std::swap(edges[id].u, edges[id].v);
}
    }

    rebuild_adj();
}

void Graph::neutralize_edge_by_id(size_t id) {
    if (id >= edges.size()) return;
    if (!edges[id].is_active) return;
    if (is_pg_edge(edges[id])) return;

    // нейтрализация ребра: выносим сопротивление Z
    r = mul_expr(r, edges[id].weight[1]);

    edges[id].is_active = false;
    rebuild_adj();
}

void Graph::contract_edge_by_id(size_t id) {
    if (id >= edges.size()) return;
    if (!edges[id].is_active) return;
    if (is_pg_edge(edges[id])) return;


    size_t u = edges[id].u;
    size_t v = edges[id].v;

    // стягивание ребра: выносим проводимость Y
    r = mul_expr(r, edges[id].weight[0]);

    edges[id].is_active = false;

    // стягиваем одну вершину в другую
    merge_vertices(u, v);
}

bool Graph::simplify_pg_degenerate_once() {
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (!is_pg_edge(edges[id])) continue;

        const Edge& e = edges[id];

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

bool Graph::simplify_loop_once() {
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (is_pg_edge(edges[id])) continue;

        Edge& e = edges[id];

        if (e.u == e.v) {
            r = mul_expr(r, e.weight[1]); // Z
            e.is_active = false;
            rebuild_adj();
            return true;
        }
    }

    return false;
}

bool Graph::simplify_leaf_once() {
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (is_pg_edge(edges[id])) continue;

        Edge& e = edges[id];

        if (e.u == e.v) continue; // петли отдельно

        if (active_degree(e.u) == 1 || active_degree(e.v) == 1) {
            r = mul_expr(r, e.weight[0]); // Y
            e.is_active = false;
            rebuild_adj();
            return true;
        }
    }

    return false;
}

bool Graph::simplify_pg_pg_series_once() {
    for (size_t x = 0; x < n; ++x) {
        if (active_degree(x) != 2) continue;

        size_t id1 = (size_t)-1;
        size_t id2 = (size_t)-1;

        for (size_t id : adj[x]) {
            if (!edges[id].is_active) continue;

            if (id1 == (size_t)-1) id1 = id;
            else {
                id2 = id;
                break;
            }
        }

        if (id1 == (size_t)-1 || id2 == (size_t)-1) continue;

        const Edge& e1 = edges[id1];
        const Edge& e2 = edges[id2];

        if (!is_pg_edge(e1) || !is_pg_edge(e2)) continue;
        if (!same_pg_pair(e1, e2)) continue;

        std::string l1 = pg_label(e1);
        std::string l2 = pg_label(e2);

        char d1 = l1.back();
        char d2 = l2.back();

        if (d1 == d2) flip_sign();

        edges[id1].is_active = false;
        edges[id2].is_active = false;
        rebuild_adj();
        return true;
    }

    return false;
}

bool Graph::simplify_series_once() {
    for (size_t x = 0; x < n; ++x) {

        if (active_degree(x) != 2) continue;

        // достаём id двух активных рёбер у вершины x
        size_t id1 = (size_t)-1;
        size_t id2 = (size_t)-1;

        for (size_t id : adj[x]) {
            if (!edges[id].is_active) continue;

            if (id1 == (size_t)-1) id1 = id;
            else { id2 = id; break; }
        }

        // на всякий случай 
        if (id1 == (size_t)-1 || id2 == (size_t)-1) continue;

        const Edge& e1 = edges[id1];
        const Edge& e2 = edges[id2];

        if (is_pg_edge(e1) || is_pg_edge(e2))
            continue;

        // внешние вершины a и b
        size_t a = (e1.u == x) ? e1.v : e1.u;
        size_t b = (e2.u == x) ? e2.v : e2.u;

        if (a == x || b == x) continue;

        // деактивируем старые
        edges[id1].is_active = false;
        edges[id2].is_active = false;

        // создаём новое ребро
        add_edge(std::min(a,b), std::max(a,b), 
            mul_expr(e1.weight[0], e2.weight[0]), 
            add_expr(mul_expr(e1.weight[0], e2.weight[1]), mul_expr(e1.weight[1], e2.weight[0])));
        rebuild_adj();
        return true;
    }

    return false;
}

bool Graph::simplify_pg_series_once() {
    for (size_t x = 0; x < n; ++x) {
        if (active_degree(x) != 2) continue;

        size_t id1 = (size_t)-1;
        size_t id2 = (size_t)-1;

        for (size_t id : adj[x]) {
            if (!edges[id].is_active) continue;

            if (id1 == (size_t)-1) {
                id1 = id;
            } else {
                id2 = id;
                break;
            }
        }

        if (id1 == (size_t)-1 || id2 == (size_t)-1) continue;

        bool pg1 = is_pg_edge(edges[id1]);
        bool pg2 = is_pg_edge(edges[id2]);

        // нужен случай: ровно одно ребро PG
        if (pg1 == pg2) continue;

        size_t reg_id = pg1 ? id2 : id1;
        Edge& reg = edges[reg_id];

        size_t other = (reg.u == x) ? reg.v : reg.u;

            this->r = mul_expr(r, reg.weight[0]);
            reg.is_active = false;

        // стягиваем вершину x в вершину other
        merge_vertices(x, other);
        return true;
    }

    return false;
}

bool Graph::simplify_parallel_once() {
    // соберём группы активных ребер по паре (u,v)
    std::map<std::pair<size_t,size_t>, std::vector<size_t>> groups;

    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (is_pg_edge(edges[id])) continue;

        size_t a = edges[id].u;
        size_t b = edges[id].v;
        if (a > b) std::swap(a,b);
        groups[{a,b}].push_back(id);
    }

    for (auto& kv : groups) {
        auto& ids = kv.second;
        if (ids.size() < 2) continue; // нет параллели

        size_t a = kv.first.first;
        size_t b = kv.first.second;

        // --- считаем новый вес ---
        // w0 = product of [1]
        std::string w0 = "1";
        for (size_t id : ids) {
            w0 = mul_expr(w0, edges[id].weight[1]);
        }

        size_t k = ids.size();
        std::vector<std::string> pref(k + 1, "1");
        std::vector<std::string> suf(k + 1, "1");

        for (size_t i = 0; i < k; ++i) {
            pref[i+1] = mul_expr(pref[i], edges[ids[i]].weight[1]);
        }
        for (size_t i = k; i-- > 0; ) {
            suf[i] = mul_expr(edges[ids[i]].weight[1], suf[i+1]);
        }

        std::string w1;
        for (size_t i = 0; i < k; ++i) {
            const auto& ei = edges[ids[i]];
            std::string others = mul_expr(pref[i], suf[i+1]);
            std::string term = mul_expr(ei.weight[0], others);
            w1 = add_expr(w1, term);
        }
        if (w1.empty()) w1 = "0";

        // деактивируем старые параллельные
        for (size_t id : ids) edges[id].is_active = false;

        add_edge(a, b, w1, w0);
        rebuild_adj();
        return true;
    }

    return false;
}

bool Graph::simplify_pg_parallel_once() {
    std::map<std::pair<size_t, size_t>, bool> has_pg;

    // сначала отмечаем пары вершин, где есть PG
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (!is_pg_edge(edges[id])) continue;

        size_t a = edges[id].u;
        size_t b = edges[id].v;
        if (a > b) std::swap(a, b);

        has_pg[{a, b}] = true;
    }

    // теперь ищем обычные рёбра, параллельные PG
    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;
        if (is_pg_edge(edges[id])) continue;

        size_t a = edges[id].u;
        size_t b = edges[id].v;
        if (a > b) std::swap(a, b);

        if (has_pg[{a, b}]) {
            
            this->r = mul_expr(r, edges[id].weight[1]);
            edges[id].is_active = false;
            rebuild_adj();
            return true;
        }
    }

    return false;
}

bool Graph::simplify_pg_pg_parallel_once() {
    for (size_t id1 = 0; id1 < edges.size(); ++id1) {
        if (!edges[id1].is_active) continue;
        if (!is_pg_edge(edges[id1])) continue;

        for (size_t id2 = id1 + 1; id2 < edges.size(); ++id2) {
            if (!edges[id2].is_active) continue;
            if (!is_pg_edge(edges[id2])) continue;

            const Edge& e1 = edges[id1];
            const Edge& e2 = edges[id2];

            if (!same_pg_pair(e1, e2)) continue;

            // должны быть параллельны
            if (!(e1.u == e2.u && e1.v == e2.v)) continue;

            std::string l1 = pg_label(e1);
            std::string l2 = pg_label(e2);

            char d1 = l1.back();
            char d2 = l2.back();

            if (d1 != d2) flip_sign();

            size_t a = e1.u;
            size_t b = e1.v;

            edges[id1].is_active = false;
            edges[id2].is_active = false;

            merge_vertices(a, b);
            return true;
        }
    }

    return false;
}

void Graph::simplify() {
    bool changed = true;
    while (changed) {

        changed = false;

        if (simplify_pg_degenerate_once()) {
            // std::cout << "pg_degenerate" << std::endl;
            return;
        }
        if (simplify_loop_once()) {
            changed = true;
            // std::cout << "loop" << std::endl;
            continue;
        }

        if (simplify_leaf_once()) {
            changed = true;
            // std::cout << "leaf" << std::endl;
            continue;
        }
        if (simplify_series_once())   { 
            changed = true; 
            // std::cout << "series" << std::endl;
            continue; 
        }
        if (simplify_parallel_once()) { 
            changed = true; 
            // std::cout << "paralel" << std::endl;
            continue; 
        }
        if (simplify_pg_parallel_once()) {
            changed = true;
            // std::cout << "pg_parallel" << std::endl;
            continue;
        }
        if (simplify_pg_series_once()) {
            changed = true;
            // std::cout << "pg_series" << std::endl;
            continue;
        }
        if (simplify_pg_pg_parallel_once()) {
            changed = true;
            // std::cout << "pg_pg_parallel" << std::endl;
            continue;
        }
        if (simplify_pg_pg_series_once()) {
            changed = true;
            // std::cout << "pg_pg_series" << std::endl;
            continue;
        }
    }
}

void Graph::rebuild_adj() {
    adj.assign(n, {});

    for (size_t id = 0; id < edges.size(); ++id) {
        if (!edges[id].is_active) continue;

        size_t u = edges[id].u;
        size_t v = edges[id].v;

        adj[u].push_back(id);
        if (u != v) {
            adj[v].push_back(id);
        }
    }
}

std::string Graph::solve_impl(size_t depth) {
    if (depth > max_recursion_depth) {
        max_recursion_depth = depth;
    }

    simplify();

    if (r == "0") {
        return "0";
    }

    if (is_solved()) {
        if (sign == -1) {
            return mul_expr("-1", r);
        }

        return r;
    }

    size_t edge_id = choose_edge_for_recursion();

    if (edge_id == static_cast<size_t>(-1)) {
        // Сюда попадём, если обычных рёбер уже нет,
        // но какие-то P/G почему-то остались и simplify их не добил.

        if (sign == -1) {
            return mul_expr("-1", r);
        }

        return r;
    }

    Graph neutralized = *this;
    Graph contracted = *this;

    neutralized.neutralize_edge_by_id(edge_id);
    contracted.contract_edge_by_id(edge_id);

    std::string res_neutralized = neutralized.solve_impl(depth + 1);
    std::string res_contracted = contracted.solve_impl(depth + 1);

    if (neutralized.get_max_recursion_depth() > max_recursion_depth) {
        max_recursion_depth = neutralized.get_max_recursion_depth();
    }
    if (contracted.get_max_recursion_depth() > max_recursion_depth) {
        max_recursion_depth = contracted.get_max_recursion_depth();
    }

    return add_expr(res_neutralized, res_contracted);
}

std::string Graph::solve() {
    max_recursion_depth = 0;
    return solve_impl(1);
}
