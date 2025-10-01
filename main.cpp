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
    std::cout << "\n" << gr.recursive_algorithm();


    return 0;
}