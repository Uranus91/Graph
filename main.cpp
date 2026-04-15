#include "graph.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    const std::string path = "test/test4.txt";
    const int iterations = 10000;

    Graph graf(path);

    // Печать исходного графа
    graf.print();

    using clock = std::chrono::high_resolution_clock;
    long long total_ns = 0;

    // Замеряем только simplify()
    for (int i = 0; i < iterations; ++i) {
        Graph temp = graf;  // копия уже считанного графа

        auto start = clock::now();
        temp.simplify();
        auto end = clock::now();

        total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

    double avg_ns = static_cast<double>(total_ns) / iterations;
    double avg_us = avg_ns / 1000.0;

    std::cout << "\n===== Timing =====\n";
    std::cout << "Average per simplify:\n";
    std::cout << "  " << avg_us << " us\n";

    // Один настоящий прогон, чтобы увидеть итоговый граф
    graf.simplify();
    graf.print();

    return 0;
}