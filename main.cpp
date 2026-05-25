#include "graph.h"
#include "rlc_scheme.h"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

void run_rlc_benchmark(const std::string& path);
void run_resistive_benchmark(const std::string& path);

int main() {
    // run_rlc_benchmark("test_rlc/test6_5.txt");
    run_resistive_benchmark("test_resistor/rec8.txt");
    return 0;
}

void run_rlc_benchmark(const std::string& path) {
    constexpr size_t benchmark_runs = 10000;

    std::cout << path << std::endl;
    RLCScheme scheme(path);

    std::cout << "Initial scheme:\n";
    scheme.print();
    std::cout << "\n";

    std::vector<long long> coeffs = scheme.solve_by_masks();
    std::string polynomial = scheme.build_polynomial(coeffs);

    volatile size_t benchmark_sink = 0;

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < benchmark_runs; ++i) {
        std::vector<long long> benchmark_coeffs = scheme.solve_by_masks();
        benchmark_sink += benchmark_coeffs.size();
    }
    auto finish = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration<double, std::micro>(finish - start);
    double average_microseconds = elapsed.count() / benchmark_runs;

    std::cout << "Result:\n";
    std::cout << polynomial << "\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Algorithm time: " << average_microseconds << " us\n";
}

void run_resistive_benchmark(const std::string& path) {
    constexpr size_t benchmark_runs = 10000;

    Graph graf(path);
    std::cout << path << std::endl;
    graf.print();

    long long result = 0;
    volatile long long benchmark_sink = 0;

    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < benchmark_runs; ++i) {
        Graph temp = graf;
        result = temp.solve();
        benchmark_sink += result;
    }
    auto finish = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration<double, std::micro>(finish - start);
    double average_microseconds = elapsed.count() / benchmark_runs;

    std::cout << "\nResult:\n";

    Graph final_graph = graf;
    result = final_graph.solve();
    std::cout << result << std::endl;
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "Algorithm time: " << average_microseconds << " us\n";

    const size_t recursion_depth = final_graph.get_max_recursion_depth();
    const size_t branch_depth = (recursion_depth == 0) ? 0 : (recursion_depth - 1);
    std::cout << "Max recursion depth: " << branch_depth << std::endl;
}
