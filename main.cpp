#include "graph.h"
#include "compare.h"
#include <iostream>
#include <chrono>
#include <string>
#include <cstddef>

// --- прототипы сверху ---
std::size_t CountAdds(const std::string& s);
std::size_t CountMuls(const std::string& s);

using Weight = std::string;

int main() {
    Graph<Weight> graf("test/input7_1.txt");
    graf.Print();

    Weight result1, result2;

    using clock = std::chrono::steady_clock;
    constexpr std::size_t ITER  = 1000;

    long long dur1 = 0;
    for (std::size_t iter = 0; iter < ITER; ++iter) {
        result1.clear();
        Graph<Weight> g1 = graf;
        auto start1 = clock::now();
        result1 = g1.GetDeterminantNonRecursive();
        auto end1 = clock::now();
        dur1 += std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count();
    }

    long long dur2 = 0;
    for (std::size_t iter = 0; iter < ITER; ++iter) {
        result2.clear();
        Graph<Weight> g2 = graf;
        auto start2 = clock::now();
        result2 = g2.GetDeterminantRecursive();
        auto end2 = clock::now();
        dur2 += std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2).count();
    }

    // --- Вывод выражений + подсчёт операций ---
    std::cout << "=== Non-recursive expression ===\n";
    std::cout << result1 << "\n";
    std::cout << "Adds: " << CountAdds(result1)
              << ", Muls: " << CountMuls(result1) << "\n\n";

    std::cout << "=== Recursive expression ===\n";
    std::cout << result2 << "\n";
    std::cout << "Adds: " << CountAdds(result2)
              << ", Muls: " << CountMuls(result2) << "\n\n";

    CompareExpressions(result1, result2);

    double avg1 = ITER ? static_cast<double>(dur1) / ITER / 1000.0 : 0.0;
    double avg2 = ITER ? static_cast<double>(dur2) / ITER / 1000.0 : 0.0;

    std::cout << "--- Performance ---\n";
    std::cout << "Non-recursive avg:   " << avg1 << " us/iter\n";
    std::cout << "Recursive avg:       " << avg2 << " us/iter\n";

    if (avg1 > 0.0) {
        std::cout << "Relative (recursive / non-recursive): " << (avg2 / avg1) << "\n";
    } else {
        std::cout << "Relative (recursive / non-recursive): N/A\n";
    }

    return 0;
}

// --- реализации снизу ---

std::size_t CountAdds(const std::string& s) {
    std::size_t cnt = 0;
    for (char c : s) {
        if (c == '+') ++cnt;
    }
    return cnt;
}

std::size_t CountMuls(const std::string& s) {
    std::size_t cnt = 0;
    for (char c : s) {
        if (c == '*') ++cnt;
    }
    return cnt;
}