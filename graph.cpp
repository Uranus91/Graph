// graph.cpp (string builder + reserve/append, логика без изменений)

#include "graph.h"
#include <iostream>
#include <fstream>
#include <queue>
#include <stdexcept>
#include <set>
#include <map>
#include <string_view>

// -----------------
// Вспомогательные функции для работы со строками
// -----------------
namespace {

inline bool needs_parens(std::string_view s) {
    for (char c : s) {
        if (c == '+' || c == ' ') return true;
    }
    return false;
}

// append factor with minimal parentheses
inline void append_wrapped(std::string& out, std::string_view s) {
    if (s.empty()) return;

    const bool already = (s.size() >= 2 && s.front() == '(' && s.back() == ')');
    const bool wrap = (!already && needs_parens(s));

    if (wrap) out.push_back('(');
    out.append(s.data(), s.size());
    if (wrap) out.push_back(')');
}

// Иногда удобнее получить factor как string (редко, но оставим)
inline std::string wrap_factor(std::string_view s) {
    if (s.empty()) return {};
    std::string out;
    out.reserve(s.size() + 2);
    append_wrapped(out, s);
    return out;
}

// "умножение" двух факторов: склейка с учётом скобок
inline std::string mul_expr(std::string_view a, std::string_view b) {
    if (a.empty()) return wrap_factor(b);
    if (b.empty()) return wrap_factor(a);

    std::string out;
    out.reserve(a.size() + b.size() + 4);
    append_wrapped(out, a);
    append_wrapped(out, b);
    return out;
}

// "сложение" двух выражений
inline std::string add_expr(std::string_view a, std::string_view b) {
    if (a.empty()) return std::string(b);
    if (b.empty()) return std::string(a);

    std::string out;
    out.reserve(a.size() + 3 + b.size());
    out.append(a.data(), a.size());
    out.append(" + ", 3);
    out.append(b.data(), b.size());
    return out;
}

// формула для параллельных ветвей при 2 активных вершинах
std::string build_parallel_expr(const Graph& g);

// состояние для рекурсии (Graph + накопленный множитель)
struct RecState {
    Graph g;
    std::string r;
};

std::string detRec(RecState st);

} // namespace


Graph::Graph(const char* filename): n(0), adj()
{
    n = ReadGraph(filename);
}

Graph::Graph(size_t n_): n(n_), adj(n_){}


void Graph::Print() const {
    std::cout << "Original graph:\n";
    for (size_t u = 0; u < n; ++u) {
        std::cout << "Vertex " << u + 1 << ": ";
        for (const auto& edge : adj[u]) {
            std::cout << "(" << edge.first + 1 << ", " << edge.second << ") ";
        }
        std::cout << "\n";
    }
    std::cout << "Unique edges: " << edges_count() << "\n";
}


size_t Graph::ReadGraph(const char* filename) {
    size_t ver = 0, edg = 0, u = 0, v = 0;
    std::string weight;

    std::ifstream inFile(filename);
    if (!inFile) {
        throw std::runtime_error("Cannot open input file");
    }

    inFile >> ver >> edg;
    GType temp(ver);

    for (size_t i = 0; i < edg; ++i) {
        inFile >> weight >> u >> v;
        if (u < 1 || u > ver || v < 1 || v > ver) {
            throw std::runtime_error("Bad vertex index in input file");
        }
        temp[u - 1].push_back({ v - 1, weight });
        temp[v - 1].push_back({ u - 1, weight });
    }

    inFile.close();
    adj = std::move(temp);
    return ver;
}

// -----------------
// Edge ops
// -----------------

void Graph::add_edge(size_t u, size_t v, std::string w) {
    if (u >= n || v >= n) return;
    adj[u].push_back({ v, std::move(w) });
    adj[v].push_back({ u, adj[u].back().second }); // копия строки как было
}

void Graph::remove_edge(size_t u, size_t v, std::string w) {
    if (u >= n || v >= n) return;

    auto erase_one = [&](size_t from, size_t to) {
        auto& vec = adj[from];
        for (auto it = vec.begin(); it != vec.end(); ) {
            if (it->first == to && it->second == w) {
                it = vec.erase(it);
                break;
            } else {
                ++it;
            }
        }
    };

    erase_one(u, v);
    erase_one(v, u);
}

// -----------------
// merge_vertex
// -----------------

void Graph::merge_vertex(size_t u, size_t v, std::string weight) {
    if (u >= n || v >= n) return;
    if (u == v) return;

    remove_edge(u, v, weight);

    GType0 v_edges = adj[v];
    for (const auto& edge : v_edges) {
        size_t neighbor = edge.first;
        const std::string& w = edge.second;

        remove_edge(v, neighbor, w);
        add_edge(u, neighbor, w);
    }
    adj[v].clear();
}

// -----------------
// simplify
// -----------------

void Graph::simplify() {
    bool changed = true;
    while (changed) {
        changed = false;

        for (size_t i = 0; i < n; ++i) {
            if (adj[i].empty()) continue;

            std::map<size_t, std::vector<std::string>> by_neighbor;
            for (const auto& e : adj[i]) {
                size_t nb = e.first;
                if (nb == i) continue;
                by_neighbor[nb].push_back(e.second);
            }

            size_t uniq = by_neighbor.size();

            if (uniq == 1) {
                auto it = by_neighbor.begin();
                size_t nb = it->first;
                const auto& ws = it->second;

                if (ws.size() == 1) {
                    remove_edge(i, nb, ws[0]);
                    changed = true;
                    break;
                }

                if (ws.size() == 2) {
                    const std::string& w1 = ws[0];
                    const std::string& w2 = ws[1];

                    remove_edge(i, nb, w1);
                    remove_edge(i, nb, w2);

                    add_edge(nb, nb, w1 + "+" + w2);
                    changed = true;
                    break;
                }
            }

            if (uniq == 2) {
                auto it = by_neighbor.begin();
                size_t u = it->first; const auto& ws1 = it->second; ++it;
                size_t v = it->first; const auto& ws2 = it->second;

                if (ws1.size() == 1 && ws2.size() == 1) {
                    const std::string& w1 = ws1[0];
                    const std::string& w2 = ws2[0];

                    remove_edge(i, u, w1);
                    remove_edge(i, v, w2);

                    add_edge(u, v, w1 + "+" + w2);
                    changed = true;
                    break;
                }
            }
        }
    }
}

// -----------------
// takeaway (дублирующиеся петли поддерживаются, как у тебя)
// -----------------

std::string Graph::takeaway() {
    std::string res;
    std::map<std::pair<size_t, std::string>, size_t> loop_count;

    for (size_t u = 0; u < n; ++u) {
        for (const auto& edge : adj[u]) {
            if (edge.first == u && !edge.second.empty()) {
                loop_count[{u, edge.second}]++;
            }
        }
    }

    for (const auto& kv : loop_count) {
        const size_t u = kv.first.first;
        const std::string& w = kv.first.second;
        const size_t cnt = kv.second;

        size_t times = cnt / 2;
        if (cnt % 2 == 1) times += 1;

        for (size_t k = 0; k < times; ++k) {
            // res = mul_expr(res, w) но быстрее: in-place append
            if (res.empty()) {
                res.reserve(w.size() + 2);
                append_wrapped(res, w);
            } else {
                res.reserve(res.size() + w.size() + 2);
                append_wrapped(res, w);
            }
        }

        size_t removed = 0;
        while (removed < cnt) {
            remove_edge(u, u, w);
            removed += 2; // как в твоей версии
        }
    }

    return res;
}

// -----------------
// counters & helpers
// -----------------

size_t Graph::edges_count() const {
    std::set<std::pair<size_t, size_t>> counter;
    for (size_t i = 0; i < n; ++i) {
        for (const auto& edge : adj[i]) {
            size_t j = edge.first;
            if (i < j) counter.insert({ i, j });
        }
    }
    return counter.size();
}

size_t Graph::vertex_count() const {
    size_t count = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!adj[i].empty()) ++count;
    }
    return count;
}

bool Graph::is_connected() const {
    if (n == 0) return true;

    std::vector<bool> visited(n, false);
    std::queue<size_t> q;

    visited[0] = true;
    q.push(0);
    size_t count = 1;

    while (!q.empty()) {
        size_t u = q.front();
        q.pop();

        for (const auto& edge : adj[u]) {
            size_t v = edge.first;
            if (!visited[v]) {
                visited[v] = true;
                ++count;
                q.push(v);
            }
        }
    }

    return count == n;
}

std::vector<std::pair<std::string, std::string>> Graph::split_by_edges() const {
    // ключ = неориентированная пара вершин
    std::map<std::pair<size_t, size_t>, std::vector<std::string>> edge_map;

    for (size_t u = 0; u < n; ++u) {
        for (const auto& e : adj[u]) {
            size_t v = e.first;
            if (u == v) continue;              // петли здесь не учитываем (их выносит takeaway)
            if (u < v) edge_map[{u, v}].push_back(e.second);
        }
    }

    std::vector<std::pair<std::string, std::string>> out;
    out.reserve(edge_map.size());

    for (auto& kv : edge_map) {
        auto& ws = kv.second;
        const size_t k = ws.size();

        // заранее сделаем "обёрнутые" факторы, чтобы не проверять '+' много раз
        std::vector<std::string> wf;
        wf.reserve(k);
        for (auto& w : ws) {
            wf.push_back(wrap_factor(w)); // использует твою needs_parens/append_wrapped
        }

        // --- prod ---
        std::string prod;
        {
            size_t cap = 0;
            for (auto& f : wf) cap += f.size();
            prod.reserve(cap);
            for (auto& f : wf) prod.append(f);
        }

        // --- par ---
        std::string par;
        if (k <= 1) {
        } else if (k == 2) {
            par.reserve(wf[0].size() + 1 + wf[1].size());
            par.append(wf[0]);
            par.push_back('+');
            par.append(wf[1]);
        } else if (k == 3) {
            auto append_prod2 = [&](size_t a, size_t b) {
                par.append(wf[a]);
                par.append(wf[b]);
            };
            // грубая оценка
            par.reserve((wf[0].size()+wf[1].size())*3 + 2);
            append_prod2(0,1); par.push_back('+');
            append_prod2(1,2); par.push_back('+');
            append_prod2(0,2);
        } else if (k == 4) {
            auto append_prod3 = [&](size_t a, size_t b, size_t c) {
                par.append(wf[a]);
                par.append(wf[b]);
                par.append(wf[c]);
            };
            par.reserve(prod.size()*4 + 3);
            append_prod3(0,1,2); par.push_back('+');
            append_prod3(0,1,3); par.push_back('+');
            append_prod3(0,2,3); par.push_back('+');
            append_prod3(1,2,3);
        } else {

            par.reserve(k * (prod.size() + 1));
            for (size_t skip = 0; skip < k; ++skip) {
                if (!par.empty()) par.push_back('+');
                for (size_t j = 0; j < k; ++j) {
                    if (j == skip) continue;
                    par.append(wf[j]);
                }
            }
        }

        out.push_back({std::move(prod), std::move(par)});
    }

    return out;
}


// -----------------
// Non-recursive determinant
// -----------------

std::string Graph::GetDeterminantNonRecursive() {
    // Лучше работать на копии, чтобы не портить исходный граф:
    // Graph g = *this; return g.GetDeterminantNonRecursive();
    // но если ты оставляешь как есть — помни, что simplify/takeaway меняют граф.

    simplify();
    std::string prefix = takeaway();

    auto groups = split_by_edges(); // vector<pair<string,string>>: {prod, par}

    std::string expr;
    expr.reserve(512);

    for (size_t i = 0; i < groups.size(); ++i) {
        std::string temp;
        temp.reserve(256);

        // string_view тут полезен: не копируем строки из groups
        std::string_view prod_i = groups[i].first;
        append_wrapped(temp, prod_i);

        for (size_t j = 0; j < groups.size(); ++j) {
            if (i == j) continue;

            std::string_view par_j = groups[j].second;
            if (par_j.empty()) continue;        // 1 ребро -> ничего не домножаем (как *1)

            // Важно: par_j это выражение с '+', значит append_wrapped сам добавит скобки
            append_wrapped(temp, par_j);
        }

        if (expr.empty()) {
            expr.push_back('(');
            expr.append(temp);
            expr.push_back(')');
        } else {
            expr.append(" + (", 4);
            expr.append(temp);
            expr.push_back(')');
        }
    }

    if (!prefix.empty()) {
        std::string out;
        out.reserve(prefix.size() + expr.size() + 4);
        out.push_back('(');
        out.append(prefix);
        out.append(")(", 2);
        out.append(expr);
        out.push_back(')');
        return out;
    }

    return expr;
}


// -----------------
// Recursive determinant
// -----------------

std::string Graph::GetDeterminantRecursive() {
    RecState st{ *this, "" };
    return detRec(st);
}

// ==============================
// anonymous namespace implementations
// ==============================
namespace {

std::string build_parallel_expr(const Graph& g) {
    std::set<std::string> weights;
    for (size_t u = 0; u < g.n; ++u) {
        if (g.adj[u].empty()) continue;
        for (const auto& edge : g.adj[u]) {
            weights.insert(edge.second);
        }
    }

    std::string result;
    result.reserve(128);

    for (const auto& skip : weights) {
        if (!result.empty()) result.push_back('+');

        std::string product;
        product.reserve(64);

        for (const auto& w : weights) {
            if (w == skip) continue;
            append_wrapped(product, w);
        }
        result.append(product);
    }

    std::string out;
    out.reserve(result.size() + 2);
    out.push_back('(');
    out.append(result);
    out.push_back(')');
    return out;
}

std::string detRec(RecState st) {
    st.g.simplify();
    std::string loops = st.g.takeaway();
    st.r = mul_expr(st.r, loops);

    if (st.g.edges_count() == 0) {
        return st.r.empty() ? std::string("1") : st.r;
    }

    if (st.g.vertex_count() == 2) {
        std::string par = build_parallel_expr(st.g);
        st.r = mul_expr(st.r, par);
        return st.r;
    }

    int u = -1, v = -1;
    std::string weight;

    for (size_t i = 0; i < st.g.n && u == -1; ++i) {
        if (st.g.adj[i].empty()) continue;
        for (const auto& edge : st.g.adj[i]) {
            u = static_cast<int>(i);
            v = static_cast<int>(edge.first);
            weight = edge.second;
            break;
        }
    }

    if (u == -1) {
        return st.r.empty() ? std::string("1") : st.r;
    }

    RecState del = st;
    del.g.remove_edge(static_cast<size_t>(u), static_cast<size_t>(v), weight);
    del.r = mul_expr(del.r, weight);
    std::string left = detRec(del);

    RecState con = st;
    con.g.merge_vertex(static_cast<size_t>(u), static_cast<size_t>(v), weight);
    std::string right = detRec(con);

    return add_expr(left, right);
}

} // namespace
