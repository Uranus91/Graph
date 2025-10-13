// main.cpp
#include "graph.h"
#include <iostream>
#include <fstream>

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
    gr.print();
    std::cout << "Unique edges: " << gr.edges_count() << std::endl;

    std::vector<std::vector<std::string>> a = gr.split_by_edges();
    std::string answer = "";
    for (int i = 0; i < a.size(); i++) {
        for (const auto& num : a[i]) {
            std::cout << num;
        }
        std::cout << "\n";
    }

    for (int i = 0; i < a.size(); i++) {
        std::string temp = "";
        for (const auto& num : a[i]) {
            temp += num;
        }
        for (int j = 0; j < a.size(); j++) {
            if (i == j)
                continue;
            if (a[j].size() == 2) {
                temp += "(" + a[j][0] + "+" + a[j][1] + ")";
            }   
            if (a[j].size() == 3) {
                temp += "(" + a[j][0] + a[j][1] + "+" + a[j][1] + a[j][2] + "+" + a[j][0] + a[j][2] + ")";
            } 
        }
        answer += (answer == "") ? ("(" + temp + ")") : (" + (" + temp + ")");
    }

        std::cout << answer;

    std::cout << "\n" << gr.recursive_algorithm();


    return 0;
}