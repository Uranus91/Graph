#include "graph.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    const std::string path = "test/input.txt";
    const int iterations = 10000;

    Graph graf(path);

    // Печать исходного графа
    graf.print();

    using clock = std::chrono::high_resolution_clock;
    long long total_ns = 0;
    std::string result;

    // Замеряем только solve()
    for (int i = 0; i < iterations; ++i) {
        Graph temp = graf;

        auto start = clock::now();
        result = temp.solve();
        auto end = clock::now();

        total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

    double avg_ns = static_cast<double>(total_ns) / iterations;
    double avg_us = avg_ns / 1000.0;

    std::cout << "\n===== Timing =====\n";
    std::cout << "Average per solve:\n";
    std::cout << "  " << avg_us << " us\n";

    Graph final_graph = graf;
    result = final_graph.solve();
    std::cout << result << std::endl;
    std::cout << "Max recursion depth: " << final_graph.get_max_recursion_depth() - 1 << std::endl;
    return 0;
}
