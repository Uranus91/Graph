// main.cpp
#include "graph.h"
#include <iostream>
#include <fstream>

int main() {
    std::ifstream inFile("input.txt");

    if (!inFile.is_open()) {
        std::cerr << "Error: Could not open file 'input.txt'." << std::endl;
        return 1;
    }

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

    if (!gr.is_connected()) {
        std::cout << "Graph is not connected." << std::endl;
        return 0;
    }

    Graph::TreeResult tree = gr.make_tree();
    tree.print();

    return 0;
}