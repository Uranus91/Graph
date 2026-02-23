#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <utility>
#include <cstddef>

// Список рёбер одной вершины: (сосед, вес)
template<typename W>
using GType0T = std::vector<std::pair<size_t, W>>;

// Весь граф: для каждой вершины — список её рёбер
template<typename W>
using GTypeT  = std::vector<GType0T<W>>;

// "Алгебра" веса: как складывать/умножать, что такое 1 и 0.
// Реализация для чисел и специализация для std::string будут в graph.cpp
template<typename W>
struct WeightOps;

// Шаблонный граф
template<typename W, typename Ops = WeightOps<W>>
class Graph {
public:
    // --- Конструкторы ---
    explicit Graph(const char* filename);
    explicit Graph(std::size_t n);

    // --- Общие методы ---
    void Print() const;
    void Empty();

    // --- Определитель ---
    W GetDeterminantRecursive();
    W GetDeterminantNonRecursive();

    // --- Вспомогательные ---
    size_t edges_count() const;
    bool        is_connected() const;
    bool CanUseNonRecursive() const;
    size_t vertex_count() const;
    std::vector<std::pair<W, W>> split_by_edges() const; // {prod, par}

    // Делаем public как у тебя, чтобы helper'ы могли читать n/adj
    size_t n = 0;
    GTypeT<W>   adj;

    // --- Внутренние методы чтения/инициализации ---
    std::size_t ReadGraph(const char* filename);

    // --- Операции над рёбрами/графом ---
    void add_edge(size_t u, size_t v, W w);
    void remove_edge(size_t u, size_t v, const W& w);

    void merge_vertex(size_t u, size_t v, const W& weight);
    void simplify();   // серия/параллель без петель (логика как была)
    W takeaway();      // вынести петли в множитель и удалить

};


extern template class Graph<std::string>;
extern template class Graph<long long int>;
extern template class Graph<long double>;

#endif // GRAPH_H
