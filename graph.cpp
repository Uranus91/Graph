#include "graph.h"

#include <iostream>
#include <fstream>
#include <queue>
#include <stdexcept>
#include <set>
#include <map>
#include <string_view>
#include <type_traits>
#include <vector>

// =============================================================
// WeightOps: общий вариант для чисел + специализация для std::string
// =============================================================

template<typename T>
struct WeightOps {
    static T one()  { return T(1); }
    static T zero() { return T(0); }

    static bool is_one(const T& x)  { return x == one(); }
    static bool is_zero(const T& x) { return x == zero(); }

    static T add(const T& a, const T& b) { return a + b; }
    static T mul(const T& a, const T& b) { return a * b; }
};

template<>
struct WeightOps<std::string> {
    static std::string one()  { return ""; }  // нейтральный множитель (как у тебя было)
    static std::string zero() { return "0"; } // если понадобится

    static bool is_one(const std::string& s)  { return s.empty(); }
    static bool is_zero(const std::string& s) { return s == "0"; }

    // Нужны ли скобки вокруг фактора при умножении:
    // если внутри есть '+' или пробелы (у тебя так исторически)
    static bool needs_parens(std::string_view s) {
        for (char c : s) {
            if (c == '+' || c == ' ') return true;
        }
        return false;
    }

    // Вставить factor в out, обернув в () при необходимости
    static void append_wrapped(std::string& out, std::string_view s) {
        if (s.empty()) return;
        const bool already = (s.size() >= 2 && s.front() == '(' && s.back() == ')');
        const bool wrap = (!already && needs_parens(s));

        if (wrap) out.push_back('(');
        out.append(s.data(), s.size());
        if (wrap) out.push_back(')');
    }

    // Вставить множитель в произведение out: если out не пуст, то добавить '*'
    static void append_mul_factor(std::string& out, std::string_view factor) {
        if (factor.empty()) return;
        if (!out.empty()) out.push_back('*');
        append_wrapped(out, factor);
    }

    // "умножение" = a * b (явный знак '*')
    static std::string mul(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        if (is_zero(a) || is_zero(b)) return zero();

        std::string out;
        out.reserve(a.size() + b.size() + 3);
        append_wrapped(out, a);
        out.push_back('*');
        append_wrapped(out, b);
        return out;
    }

    // "сложение" = a + b (явный знак '+')
    static std::string add(const std::string& a, const std::string& b) {
        if (a.empty()) return b;
        if (b.empty()) return a;
        if (is_zero(a)) return b;
        if (is_zero(b)) return a;

        std::string out;
        out.reserve(a.size() + b.size() + 1);
        out.append(a);
        out.push_back('+');
        out.append(b);
        return out;
    }
};

// =============================================================
// Локальные helper-функции для аккуратных скобок в сумме
// =============================================================

// Нужно ли оборачивать СЛАГАЕМОЕ в скобки внутри суммы: (term) + (term2)
// Мы не хотим лишние скобки, но обязаны оборачивать, если в term есть '+' на верхнем уровне.
static bool needs_sum_parens_top_level(std::string_view s) {
    int depth = 0;
    for (char c : s) {
        if (c == '(') ++depth;
        else if (c == ')') { if (depth > 0) --depth; }
        else if (c == '+' && depth == 0) return true;
    }
    return false;
}

static void append_sum_term(std::string& sum, const std::string& term) {
    if (term.empty()) return;
    if (needs_sum_parens_top_level(term)) {
        sum.push_back('(');
        sum.append(term);
        sum.push_back(')');
    } else {
        sum.append(term);
    }
}

// =============================================================
// Helpers (вместо anonymous namespace): рекурсия и база n==2
// =============================================================
namespace graph_detail {

template<typename GraphT>
static std::size_t degree_no_loops(const GraphT& g, std::size_t u) {
    std::size_t deg = 0;
    for (const auto& e : g.adj[u]) {
        if (e.first == u) continue;
        ++deg;
    }
    return deg;
}

// Выбор ребра по эвристике "вершина минимальной степени"
template<typename W, typename GraphT>
static bool PickEdge_MinDegree(const GraphT& g,
                               std::size_t& out_u,
                               std::size_t& out_v,
                               W& out_w)
{
    const std::size_t INF = static_cast<std::size_t>(-1);

    std::size_t best_u = INF;
    std::size_t best_deg = INF;

    for (std::size_t u = 0; u < g.n; ++u) {
        if (g.adj[u].empty()) continue;
        std::size_t deg = degree_no_loops(g, u);
        if (deg == 0) continue;

        if (deg < best_deg) {
            best_deg = deg;
            best_u = u;
            if (best_deg == 1) break;
        }
    }

    if (best_u == INF) return false;

    for (const auto& e : g.adj[best_u]) {
        if (e.first == best_u) continue;
        out_u = best_u;
        out_v = e.first;
        out_w = e.second;
        return true;
    }

    return false;
}

// Состояние для рекурсивного детерминанта
template<typename W, typename Ops, typename GraphT>
struct RecState {
    GraphT g;
    W r; // накопленный множитель
};

// Для базы vertex_count()==2: строим sum_{skip} prod_{j!=skip}
// Важно: НЕ используем set для строк, чтобы не терять повторы.
template<typename W, typename Ops, typename GraphT>
static W build_parallel_expr(const GraphT& g) {
    // найдём 2 активные вершины
    std::vector<std::size_t> verts;
    verts.reserve(2);
    for (std::size_t i = 0; i < g.n; ++i) {
        if (!g.adj[i].empty()) verts.push_back(i);
        if (verts.size() == 2) break;
    }
    if (verts.size() < 2) return Ops::one();

    std::size_t a = verts[0], b = verts[1];

    // соберём веса рёбер между a и b (без петель)
    std::vector<W> ws;
    for (const auto& e : g.adj[a]) {
        if (e.first == b) ws.push_back(e.second);
    }

    const std::size_t k = ws.size();
    if (k <= 1) return Ops::one();

    // par = sum_{skip} prod_{j!=skip}
    W par{};
    bool first = true;
    for (std::size_t skip = 0; skip < k; ++skip) {
        W p = Ops::one();
        for (std::size_t j = 0; j < k; ++j) {
            if (j == skip) continue;
            p = Ops::mul(p, ws[j]);
        }
        if (first) { par = p; first = false; }
        else       { par = Ops::add(par, p); }
    }

    // для строк хочется скобки вокруг суммы
    if constexpr (std::is_same_v<W, std::string>) {
        if (!par.empty() && !(par.size() >= 2 && par.front() == '(' && par.back() == ')')) {
            return std::string("(") + par + ")";
        }
    }
    return par;
}

// Рекурсивный детерминант
template<typename W, typename Ops, typename GraphT>
W detRec(RecState<W, Ops, GraphT> st) {
    st.g.simplify();
    st.r = Ops::mul(st.r, st.g.takeaway());

    if (st.g.edges_count() == 0) {
        return st.r;
    }

    if (st.g.vertex_count() == 2) {
        W par = build_parallel_expr<W, Ops>(st.g);
        st.r = Ops::mul(st.r, par);
        return st.r;
    }

    std::size_t u = 0, v = 0;
    W weight = Ops::zero();

    if (!PickEdge_MinDegree(st.g, u, v, weight)) {
        return st.r;
    }

    auto del = st;
    del.g.remove_edge(u, v, weight);
    del.r = Ops::mul(del.r, weight);
    W left = detRec<W, Ops>(del);

    auto con = st;
    con.g.merge_vertex(u, v, weight);
    W right = detRec<W, Ops>(con);

    return Ops::add(left, right);
}

} // namespace graph_detail


// =============================================================
// Реализация методов Graph<W, Ops>
// =============================================================

template<typename W, typename Ops>
Graph<W, Ops>::Graph(const char* filename) : n(0), adj() {
    n = ReadGraph(filename);
}

template<typename W, typename Ops>
Graph<W, Ops>::Graph(size_t n_) : n(n_), adj(n_) {}

template<typename W, typename Ops>
void Graph<W, Ops>::Empty() {
    n = 0;
    adj.clear();
}

template<typename W, typename Ops>
void Graph<W, Ops>::Print() const {
    std::cout << "Graph:\n";
    for (size_t u = 0; u < n; ++u) {
        std::cout << "Vertex " << (u + 1) << ": ";
        for (const auto& edge : adj[u]) {
            std::cout << "(" << (edge.first + 1) << ", " << edge.second << ") ";
        }
        std::cout << "\n";
    }
    std::cout << "Unique edges: " << edges_count() << "\n";
}

template<typename W, typename Ops>
size_t Graph<W, Ops>::ReadGraph(const char* filename) {
    size_t ver = 0, edg = 0, u = 0, v = 0;

    std::ifstream inFile(filename);
    if (!inFile) throw std::runtime_error("Cannot open input file");

    inFile >> ver >> edg;
    if (!inFile) throw std::runtime_error("Bad input header");

    GTypeT<W> temp(ver);

    for (size_t i = 0; i < edg; ++i) {
        W weight{};
        inFile >> weight >> u >> v;

        if (!inFile) throw std::runtime_error("Bad edge line in input file");
        if (u < 1 || u > ver || v < 1 || v > ver) {
            throw std::runtime_error("Bad vertex index in input file");
        }

        temp[u - 1].push_back({ v - 1, weight });
        temp[v - 1].push_back({ u - 1, weight });
    }

    adj = std::move(temp);
    return ver;
}

// -----------------
// Edge ops
// -----------------

template<typename W, typename Ops>
void Graph<W, Ops>::add_edge(size_t u, size_t v, W w) {
    if (u >= n || v >= n) return;
    adj[u].push_back({ v, std::move(w) });
    adj[v].push_back({ u, adj[u].back().second });
}

template<typename W, typename Ops>
void Graph<W, Ops>::remove_edge(size_t u, size_t v, const W& w) {
    if (u >= n || v >= n) return;

    auto erase_one = [&](size_t from, size_t to) {
        auto& vec = adj[from];
        for (auto it = vec.begin(); it != vec.end(); ++it) {
            if (it->first == to && it->second == w) {
                vec.erase(it);
                break;
            }
        }
    };

    erase_one(u, v);
    erase_one(v, u);
}

// -----------------
// merge_vertex
// -----------------

template<typename W, typename Ops>
void Graph<W, Ops>::merge_vertex(size_t u, size_t v, const W& weight) {
    if (u >= n || v >= n) return;
    if (u == v) return;

    remove_edge(u, v, weight);

    auto v_edges = adj[v]; // копия (как у тебя)
    for (const auto& edge : v_edges) {
        size_t neighbor = edge.first;
        const W& w = edge.second;

        remove_edge(v, neighbor, w);
        add_edge(u, neighbor, w);
    }
    adj[v].clear();
}

// -----------------
// simplify (логика как была; для string add = "+", для чисел add = +)
// -----------------

template<typename W, typename Ops>
void Graph<W, Ops>::simplify() {
    bool changed = true;
    while (changed) {
        changed = false;

        for (size_t i = 0; i < n; ++i) {
            if (adj[i].empty()) continue;

            std::map<size_t, std::vector<W>> by_neighbor;
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
                    const W& w1 = ws[0];
                    const W& w2 = ws[1];

                    remove_edge(i, nb, w1);
                    remove_edge(i, nb, w2);

                    add_edge(nb, nb, Ops::add(w1, w2));
                    changed = true;
                    break;
                }
            }

            if (uniq == 2) {
                auto it = by_neighbor.begin();
                size_t u = it->first; const auto& ws1 = it->second; ++it;
                size_t v = it->first; const auto& ws2 = it->second;

                if (ws1.size() == 1 && ws2.size() == 1) {
                    const W& w1 = ws1[0];
                    const W& w2 = ws2[0];

                    remove_edge(i, u, w1);
                    remove_edge(i, v, w2);

                    add_edge(u, v, Ops::add(w1, w2));
                    changed = true;
                    break;
                }
            }
        }
    }
}

// -----------------
// takeaway (выносим петли в множитель и удаляем)
// -----------------

template<typename W, typename Ops>
W Graph<W, Ops>::takeaway() {
    W res = Ops::one();
    std::map<std::pair<size_t, W>, size_t> loop_count;

    for (size_t u = 0; u < n; ++u) {
        for (const auto& edge : adj[u]) {
            if (edge.first == u) {
                loop_count[{u, edge.second}]++;
            }
        }
    }

    for (const auto& kv : loop_count) {
        const size_t u = kv.first.first;
        const W& w = kv.first.second;
        const size_t cnt = kv.second;

        size_t times = cnt / 2;
        if (cnt % 2 == 1) times += 1;

        for (size_t k = 0; k < times; ++k) {
            res = Ops::mul(res, w);
        }

        size_t removed = 0;
        while (removed < cnt) {
            remove_edge(u, u, w);
            removed += 2;
        }
    }

    return res;
}

// -----------------
// counters & helpers
// -----------------

template<typename W, typename Ops>
size_t Graph<W, Ops>::edges_count() const {
    std::set<std::pair<size_t, size_t>> counter;
    for (size_t i = 0; i < n; ++i) {
        for (const auto& edge : adj[i]) {
            size_t j = edge.first;
            if (i < j) counter.insert({ i, j });
        }
    }
    return counter.size();
}

template<typename W, typename Ops>
size_t Graph<W, Ops>::vertex_count() const {
    size_t count = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!adj[i].empty()) ++count;
    }
    return count;
}

template<typename W, typename Ops>
bool Graph<W, Ops>::CanUseNonRecursive() const {
    // ты говорил: после разбиения/нормализации петель уже нет,
    // поэтому минимальный критерий:
    return this->edges_count() == this->vertex_count();
}

template<typename W, typename Ops>
bool Graph<W, Ops>::is_connected() const {
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

// -----------------
// split_by_edges -> возвращает {prod, par}
// prod = произведение всех весов на ветви u-v
// par  = сумма произведений "все кроме одного" (для параллельных)
// -----------------

template<typename W, typename Ops>
std::vector<std::pair<W, W>> Graph<W, Ops>::split_by_edges() const {
    std::map<std::pair<size_t, size_t>, std::vector<W>> edge_map;

    for (size_t u = 0; u < n; ++u) {
        for (const auto& e : adj[u]) {
            size_t v = e.first;
            if (u == v) continue;
            if (u < v) edge_map[{u, v}].push_back(e.second);
        }
    }

    std::vector<std::pair<W, W>> out;
    out.reserve(edge_map.size());

    for (auto& kv : edge_map) {
        auto& ws = kv.second;
        const size_t k = ws.size();

        if constexpr (std::is_same_v<W, std::string>) {
            // --- STRING: in-place и со '*' ---
            std::string prod;
            prod.reserve(32);
            for (const auto& w : ws) {
                WeightOps<std::string>::append_mul_factor(prod, std::string_view(w));
            }

            std::string par;
            if (k <= 1) {
                par = ""; // one()
            } else {
                par.push_back('(');
                bool first = true;
                for (size_t skip = 0; skip < k; ++skip) {
                    if (!first) par.push_back('+');
                    first = false;

                    std::string term;
                    term.reserve(32);
                    for (size_t j = 0; j < k; ++j) {
                        if (j == skip) continue;
                        WeightOps<std::string>::append_mul_factor(term, std::string_view(ws[j]));
                    }
                    par.append(term);
                }
                par.push_back(')');
            }

            out.push_back({ std::move(prod), std::move(par) });
        } else {
            // --- NUMERIC/GENERIC ---
            W prod = Ops::one();
            for (const auto& w : ws) prod = Ops::mul(prod, w);

            W par = Ops::one();
            if (k <= 1) {
                par = Ops::one();
            } else if (k == 2) {
                par = Ops::add(ws[0], ws[1]);
            } else if (k == 3) {
                W t01 = Ops::mul(ws[0], ws[1]);
                W t12 = Ops::mul(ws[1], ws[2]);
                W t02 = Ops::mul(ws[0], ws[2]);
                par = Ops::add(Ops::add(t01, t12), t02);
            } else if (k == 4) {
                W t012 = Ops::mul(Ops::mul(ws[0], ws[1]), ws[2]);
                W t013 = Ops::mul(Ops::mul(ws[0], ws[1]), ws[3]);
                W t023 = Ops::mul(Ops::mul(ws[0], ws[2]), ws[3]);
                W t123 = Ops::mul(Ops::mul(ws[1], ws[2]), ws[3]);
                par = Ops::add(Ops::add(t012, t013), Ops::add(t023, t123));
            } else {
                bool first = true;
                for (size_t skip = 0; skip < k; ++skip) {
                    W p = Ops::one();
                    for (size_t j = 0; j < k; ++j) {
                        if (j == skip) continue;
                        p = Ops::mul(p, ws[j]);
                    }
                    if (first) { par = p; first = false; }
                    else       { par = Ops::add(par, p); }
                }
            }

            out.push_back({ std::move(prod), std::move(par) });
        }
    }

    return out;
}

// -----------------
// Non-recursive determinant
// -----------------

template<typename W, typename Ops>
W Graph<W, Ops>::GetDeterminantNonRecursive() {
    simplify();
    W prefix = takeaway();
    auto groups = split_by_edges(); // vector<pair<W,W>> = {prod, par}

    if constexpr (std::is_same_v<W, std::string>) {
        // --- Быстрый строковый вывод: всё произведение с '*' ---
        std::string expr;
        expr.reserve(512);

        for (size_t i = 0; i < groups.size(); ++i) {
            std::string temp;
            temp.reserve(256);

            // prod_i
            WeightOps<std::string>::append_mul_factor(temp, std::string_view(groups[i].first));

            // * par_j
            for (size_t j = 0; j < groups.size(); ++j) {
                if (i == j) continue;
                const std::string& par_j = groups[j].second;
                if (par_j.empty()) continue;
                WeightOps<std::string>::append_mul_factor(temp, std::string_view(par_j));
            }

            if (expr.empty()) {
                append_sum_term(expr, temp);
            } else {
                expr.push_back('+');
                append_sum_term(expr, temp);
            }
        }

        if (prefix.empty()) return expr;

        // (prefix)*(expr)
        std::string out;
        out.reserve(prefix.size() + expr.size() + 5);
        out.push_back('(');
        out.append(prefix);
        out.append(")*(");
        out.append(expr);
        out.push_back(')');
        return out;
    } else {
        // --- Числовой / общий путь ---
        W expr = Ops::zero();
        bool first_term = true;

        for (size_t i = 0; i < groups.size(); ++i) {
            W temp = groups[i].first;

            for (size_t j = 0; j < groups.size(); ++j) {
                if (i == j) continue;
                const W& par_j = groups[j].second;
                if (Ops::is_one(par_j)) continue;
                temp = Ops::mul(temp, par_j);
            }

            if (first_term) { expr = temp; first_term = false; }
            else            { expr = Ops::add(expr, temp); }
        }

        if (Ops::is_one(prefix)) return expr;
        return Ops::mul(prefix, expr);
    }
}

// -----------------
// Recursive determinant
// -----------------

template<typename W, typename Ops>
W Graph<W, Ops>::GetDeterminantRecursive() {
    graph_detail::RecState<W, Ops, Graph<W, Ops>> st{ *this, Ops::one() };
    return graph_detail::detRec<W, Ops>(st);
}

// =============================================================
// Явные инстанциации (КРИТИЧНО для схемы "шаблон в .cpp")
// =============================================================
template class Graph<std::string>;
template class Graph<long long int>;
template class Graph<long double>;