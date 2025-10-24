#include "graph.h"
#include <iostream>
#include <fstream>
#include <chrono>

int main() {
    std::ifstream inFile("input.txt");

    int ver, edg;
    inFile >> ver >> edg;

    Graph gr(ver);
    int u, v;
    std::string weight;

    for (int i = 0; i < edg; ++i) {
        inFile >> u >> v >> weight;
        gr.add_edge(u - 1, v - 1, weight);
    }

    inFile.close();

    std::cout << "Original graph:" << std::endl;
    gr.print();
    gr.simplify();
    std::cout << "Unique edges: " << gr.edges_count() << std::endl;

    // === Получаем split_by_edges и выводим группы ДО замера ===
    std::vector<std::vector<std::string>> a = gr.split_by_edges();
    for (int i = 0; i < a.size(); i++) {
        for (const auto& num : a[i]) {
            std::cout << num;
        }
        std::cout << "\n";
    }

    // === Замер первого выражения (1000 итераций) ===
    auto start1 = std::chrono::high_resolution_clock::now();
    std::string answer; // будем использовать последнюю итерацию для вывода
    for (int iter = 0; iter < 10000; ++iter) {
        answer = ""; // ОЧИЩАЕМ ВНЕШНЮЮ переменную
        for (int i = 0; i < a.size(); i++) {
            std::string temp = "";
            for (const auto& num : a[i]) {
                temp += num;
            }
            for (int j = 0; j < a.size(); j++) {
                if (i == j) continue;
                if (a[j].size() == 2) {
                    temp += "(" + a[j][0] + "+" + a[j][1] + ")";
                }   
                if (a[j].size() == 3) {
                    temp += "(" + a[j][0] + a[j][1] + "+" + a[j][1] + a[j][2] + "+" + a[j][0] + a[j][2] + ")";
                } 
            }
            answer += (answer == "") ? ("(" + temp + ")") : (" + (" + temp + ")");
        }
    }
    auto end1 = std::chrono::high_resolution_clock::now();

    // === Замер второго выражения (1000 итераций) ===

    

    auto start2 = std::chrono::high_resolution_clock::now();
    std::string result2;
    for (int iter = 0; iter < 1; ++iter) {
        Graph gr1 = gr;
        result2 = gr1.recursive_algorithm(); // сохраняем в ВНЕШНЮЮ переменную
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    // === Вывод результатов (после замеров) ===
    std::cout << answer << std::endl;
    std::cout << result2 << std::endl;

    // === Вывод времени ===
    auto duration1 = std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1);
    auto duration2 = std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2);

    std::cout << "\n--- Performance ---" << std::endl;
    std::cout << "Time to generate first expression (answer): " 
              << duration1.count() << " nanoseconds (for 10000 runs)" << std::endl;
    std::cout << "Time to generate second expression (recursive_algorithm): " 
              << duration2.count() << " nanoseconds (for 10000 runs)" << std::endl;
    std::cout << (double)duration2.count() / duration1.count();

    return 0;
}