#include "graph.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <string>

int main() {
    Graph graf("test/sign_check.txt");
    graf.print();
    graf.simplify();
    graf.print();

}