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

    if (!gr.is_connected()) {
        std::cout << "Graph is not connected." << std::endl;
        return 0;
    }
    gr.simplify();
    std::cout << "\nSimplyfied:\n";
    gr.print();
    std::cout << std::endl;
    
    Graph gr1 = gr;
    gr1.remove_edge(0, 1, "R1");
    gr1.r = "R1";
    Graph gr2 = gr1;
    gr2.merge_vertex(0, 1, "R1");
    // gr1.simplify();
    // gr2.simplify();
    gr1.print();
    std::cout << gr1.degree(2) << std::endl;
    gr2.print();


    // Graph::TreeResult tree = gr.make_tree();
    // tree.print();

    return 0;
}