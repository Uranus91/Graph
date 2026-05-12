#include "graph.h"
#include "rlc_scheme.h"
#include <iostream>
#include <chrono>
#include <string>

int main() {
    RLCScheme scheme("test_rlc/input2.txt");

    std::vector<std::string> coeffs = scheme.solve_by_masks();

    std::cout << "Polynomial coefficients:\n";

    for (size_t i = coeffs.size(); i != 0; i--) {
        std::cout << "A" << i - 1 << " = " << coeffs[i - 1] << "\n";
    }

    std::string polynomial = scheme.build_polynomial();

    std::cout << "Polynomial:\n";
    std::cout << polynomial << "\n";

    return 0;

}
