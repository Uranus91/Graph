#include "graph.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <string>

int main() {
    Graph graf("test/seriespg.txt");
    graf.print();
    graf.simplify();
    graf.print();


}