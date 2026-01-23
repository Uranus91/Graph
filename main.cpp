#include "graph.h"
#include "compare.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <string>



int main() {
    Graph graf("test/input6_2.txt");
    graf.Print();  // Для наглядности: исходный граф (вне замеров)

    std::string result1, result2;
    
    using clock = std::chrono::steady_clock;
    constexpr size_t ITER  = 1000;

    // === Замер 1: нерекурсивный метод ===
    long long dur1 = 0;
    for (size_t iter = 0; iter < ITER; ++iter) {
        result1.clear();
        Graph g1 = graf;
        auto start1 = clock::now();
        result1 = g1.GetDeterminantNonRecursive();
        auto end1 = clock::now();

        dur1 += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
    }

    // === Замер 2: рекурсивный метод ===
    long long dur2 = 0;
    for (size_t iter = 0; iter < ITER; ++iter) {
        result2.clear();
        Graph g2 = graf;
        auto start2 = clock::now();
        result2 = g2.GetDeterminantRecursive();
        auto end2 = clock::now();

        dur2 += std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();
    }

    // --- Вывод выражений ---
    std::cout << "=== Non-recursive expression ===\n";
    std::cout << result1 << "\n\n";

    std::cout << "=== Recursive expression ===\n";
    std::cout << result2 << "\n\n";

    CompareExpressions(result1, result2);
    double avg1 = ITER ? static_cast<double>(dur1) / ITER    / 1000.0 : 0.0;
    double avg2 = ITER ? static_cast<double>(dur2) / ITER / 1000.0 : 0.0;

    std::cout << "--- Performance ---\n";
    std::cout << "Non-recursive avg:   " << avg1 << " us/iter\n";
    std::cout << "Recursive avg:       " << avg2 << " us/iter\n";

    if (avg1 > 0.0) {
        std::cout << "Relative (recursive / non-recursive): "
                  << (avg2 / avg1) << "\n";
    } else {
        std::cout << "Relative (recursive / non-recursive): N/A (non-recursive avg is 0)\n";
    }

    return 0;
}
