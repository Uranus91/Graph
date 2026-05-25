#ifndef RLC_SCHEME_H
#define RLC_SCHEME_H

#include "graph.h"

#include <bitset>

#include <cstddef>
#include <string>
#include <vector>


constexpr size_t MAX_REACTIVE = 32;
using SchemeMask = std::bitset<MAX_REACTIVE>;


enum class ElementType {
    Resistor,
    Capacitor,
    Inductor
};

struct GeneratedScheme {
    Graph graph;
    std::string multiplier;
    size_t p_degree = 0;
};

struct RLCElement {
    size_t u = 0;
    size_t v = 0;

    std::string name;
    ElementType type = ElementType::Resistor;

    bool is_active = true;
};

class RLCScheme {
private:
    size_t n = 0;
    size_t reactive_count = 0;
    std::vector<RLCElement> elements;
    std::vector<std::vector<size_t>> adj;

public:
    explicit RLCScheme(const std::string& file_path);
    explicit RLCScheme(size_t n_vertices);

    size_t vertex_count() const;
    size_t element_count() const;
    size_t get_reactive_count() const;

    const std::vector<RLCElement>& get_elements() const;
    const RLCElement& get_element(size_t id) const;

    size_t add_element(size_t u, size_t v, const std::string& name, ElementType type);

    void deactivate_element(size_t id);
    void merge_vertices(size_t from, size_t to);
    void rebuild_adj();

    size_t active_degree(size_t x) const;

    GeneratedScheme build_by_mask(const SchemeMask& mask) const;

    std::vector<std::string> solve_by_masks() const;

    std::string build_polynomial(const std::vector<std::string>& coeffs) const;
    std::string build_polynomial() const;

    void print() const;
};

#endif
