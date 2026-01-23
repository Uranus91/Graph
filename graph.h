#ifndef GRAPH_H
#define GRAPH_H

#include <vector>
#include <string>
#include <utility>

// Список рёбер одной вершины: (сосед, "вес")
using GType0 = std::vector<std::pair<size_t, std::string>>;
// Весь граф: для каждой вершины — список её рёбер
using GType  = std::vector<GType0>;

class Graph {
public:
    // --- Конструкторы ---
    explicit Graph(const char* filename);
    explicit Graph(size_t n);

    // --- Общие методы ---
    void Print() const;
    void Empty();

    // --- Определитель ---
    std::string GetDeterminantRecursive();
    std::string GetDeterminantNonRecursive();

    // --- Вспомогательные ---
    size_t edges_count() const;
    bool   is_connected() const;
    size_t vertex_count() const;
    std::vector<std::pair<std::string, std::string>> split_by_edges() const;

    // ⚠️ Сделал public, чтобы рекурсивный helper в graph.cpp мог
    // читать граф (n/adj) без friend-ов.
    // Если хочешь, потом легко вернём в private через friend.
    size_t n = 0;
    GType  adj;
 // можно оставить, но рекурсивная версия в целом не зависит от него

    // --- Внутренние методы чтения/инициализации ---
    size_t ReadGraph(const char* filename);

    // --- Операции над рёбрами/графом ---
    void add_edge(size_t u, size_t v, std::string w);
    void remove_edge(size_t u, size_t v, std::string w);

    void merge_vertex(size_t u, size_t v, std::string weight);
    void simplify();           // серия/параллель без петель
    std::string takeaway();    // вынести петли в строку-множитель и удалить их

};

#endif // GRAPH_H
