#include "graph.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <string>

int main() {
    Graph graf("test/input_with_ind.txt");
    graf.print();
    graf.simplify();
    graf.print();

}