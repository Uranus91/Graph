#include "graph.h"
#include "rlc_scheme.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    // const std::string path = "test/input.txt";
    // const int iterations = 10000;

    // Graph graf(path);

    // // Печать исходного графа
    // graf.print();

    
    // using clock = std::chrono::high_resolution_clock;
    // long long total_ns = 0;
    // std::string result;

    // // Замеряем только solve()
    // for (int i = 0; i < iterations; ++i) {
    //     Graph temp = graf;

    //     auto start = clock::now();
    //     result = temp.solve();
    //     auto end = clock::now();

    //     total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    // }

    // double avg_ns = static_cast<double>(total_ns) / iterations;
    // double avg_us = avg_ns / 1000.0;

    // std::cout << "\n===== Timing =====\n";
    // std::cout << "Average per solve:\n";
    // std::cout << "  " << avg_us << " us\n";

    // Graph final_graph = graf;
    // result = final_graph.solve();
    // std::cout << result << std::endl;
    // const size_t recursion_depth = final_graph.get_max_recursion_depth();
    // const size_t branch_depth = (recursion_depth == 0) ? 0 : (recursion_depth - 1);
    // std::cout << "Max recursion depth: " << branch_depth << std::endl;


    RLCScheme scheme("test_rlc/input.txt");

    std::cout << "===== Original RLC scheme =====\n";
    scheme.print();

    SchemeMask mask;

    // Проверка маски 0000:
    // все элементы выделены.
    // C1, C2 -> провод
    // L1, L2 -> разрыв
    mask.reset();

    GeneratedScheme generated = scheme.build_by_mask(mask);

    std::cout << "\n===== Generated resistive graph for mask 0000 =====\n";
    std::cout << "Multiplier: " << generated.multiplier << "\n";
    std::cout << "p degree: " << generated.p_degree << "\n";
    generated.graph.print();

    mask.reset();
    mask.set(0);
    mask.set(3);

    GeneratedScheme generated2 = scheme.build_by_mask(mask);

    std::cout << "\n===== Generated resistive graph for mask 0011 =====\n";
    std::cout << "Multiplier: " << generated2.multiplier << "\n";
    std::cout << "p degree: " << generated2.p_degree << "\n";
    generated2.graph.print();
    std::cout << generated2.graph.solve();

    return 0;
}
