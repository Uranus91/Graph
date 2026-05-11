#include "rlc_scheme.h"

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

static ElementType parse_type(const std::string& name) {
    if (name.empty()) {
        throw std::runtime_error("Empty element name");
    }

    switch (name[0]) {
        case 'R':
            return ElementType::Resistor;
        case 'C':
            return ElementType::Capacitor;
        case 'L':
            return ElementType::Inductor;
        default:
            throw std::runtime_error("Unknown element type: " + name);
    }
}

static std::string type_to_string(ElementType type) {
    switch (type) {
        case ElementType::Resistor:
            return "R";
        case ElementType::Capacitor:
            return "C";
        case ElementType::Inductor:
            return "L";
    }

    return "?";
}

static std::string mul_string(const std::string& a, const std::string& b) {
    if (a.empty() || a == "1") return b;
    if (b.empty() || b == "1") return a;
    if (a == "0" || b == "0") return "0";
    return a + "*" + b;
}

static std::string add_string(const std::string& a, const std::string& b) {
    if (a.empty() || a == "0") return b;
    if (b.empty() || b == "0") return a;
    return a + "+" + b;
}

RLCScheme::RLCScheme(size_t n_vertices) : n(n_vertices), adj(n_vertices) {}

RLCScheme::RLCScheme(const std::string& file_path) {
    std::ifstream in(file_path);

    if (!in) {
        throw std::runtime_error("Cannot open file: " + file_path);
    }

    size_t m = 0;
    in >> n >> m;

    if (!in) {
        throw std::runtime_error("Bad header read");
    }

    elements.clear();
    adj.assign(n, {});
    elements.reserve(m);
    reactive_count = 0;

    for (size_t i = 0; i < m; ++i) {
        std::string name;
        size_t u = 0;
        size_t v = 0;

        in >> name >> u >> v;

        if (!in) {
            throw std::runtime_error(
                "Bad element read at line " + std::to_string(i + 2)
            );
        }

        if (u == 0 || v == 0 || u > n || v > n) {
            throw std::runtime_error(
                "Vertex index out of range at line " + std::to_string(i + 2)
            );
        }

        add_element(u - 1, v - 1, name, parse_type(name));
    }
}

size_t RLCScheme::vertex_count() const {
    return n;
}

size_t RLCScheme::element_count() const {
    return elements.size();
}

size_t RLCScheme::get_reactive_count() const {
    return reactive_count;
}

const std::vector<RLCElement>& RLCScheme::get_elements() const {
    return elements;
}

const RLCElement& RLCScheme::get_element(size_t id) const {
    return elements[id];
}

size_t RLCScheme::add_element(size_t u, size_t v, const std::string& name, ElementType type) {
    if (u >= n || v >= n) {
        throw std::runtime_error("add_element(): vertex index out of range");
    }

    RLCElement e;
    e.u = u;
    e.v = v;
    e.name = name;
    e.type = type;
    e.is_active = true;

    size_t id = elements.size();
    elements.push_back(std::move(e));

        if (type == ElementType::Capacitor || type == ElementType::Inductor) {
        ++reactive_count;
    }

    adj[u].push_back(id);
    if (u != v) {
        adj[v].push_back(id);
    }

    return id;
}

void RLCScheme::deactivate_element(size_t id) {
    if (id >= elements.size()) {
        throw std::runtime_error("deactivate_element(): bad element id");
    }

    elements[id].is_active = false;
    rebuild_adj();
}

void RLCScheme::merge_vertices(size_t from, size_t to) {
    if (from == to) return;

    if (from >= n || to >= n) {
        throw std::runtime_error("merge_vertices(): vertex index out of range");
    }

    for (auto& e : elements) {
        if (!e.is_active) continue;

        if (e.u == from) e.u = to;
        if (e.v == from) e.v = to;

        if (e.u > e.v) {
            std::swap(e.u, e.v);
        }
    }

    rebuild_adj();
}

void RLCScheme::rebuild_adj() {
    adj.assign(n, {});

    for (size_t id = 0; id < elements.size(); ++id) {
        if (!elements[id].is_active) continue;

        size_t u = elements[id].u;
        size_t v = elements[id].v;

        adj[u].push_back(id);
        if (u != v) {
            adj[v].push_back(id);
        }
    }
}

size_t RLCScheme::active_degree(size_t x) const {
    if (x >= n) {
        throw std::runtime_error("active_degree(): vertex index out of range");
    }

    size_t deg = 0;

    for (size_t id : adj[x]) {
        if (elements[id].is_active) {
            ++deg;
        }
    }

    return deg;
}

GeneratedScheme RLCScheme::build_by_mask(const SchemeMask& mask) const {
    if (reactive_count > MAX_REACTIVE) {
        throw std::runtime_error("Too many reactive elements for SchemeMask");
    }

    RLCScheme transformed = *this;

    std::string multiplier = "1";
    size_t p_degree = 0;

    size_t reactive_index = 0;

    // Фаза 1: применяем C/L по маске к копии схемы.
    for (size_t id = 0; id < transformed.elements.size(); ++id) {
        RLCElement& e = transformed.elements[id];

        if (!e.is_active) continue;

        if (e.type == ElementType::Resistor) {
            continue;
        }

        bool bit = mask[reactive_index];

        // bit = 0 -> выделение в множитель
        // bit = 1 -> нейтрализация
        bool is_selected = !bit;

        if (is_selected) {
            multiplier = mul_string(multiplier, e.name);
            ++p_degree;
        }

        size_t u = e.u;
        size_t v = e.v;

        if (e.type == ElementType::Capacitor) {
            if (is_selected) {
                // C, bit=0: выделение -> провод
                transformed.deactivate_element(id);
                transformed.merge_vertices(u, v);
            } else {
                // C, bit=1: нейтрализация -> разрыв
                transformed.deactivate_element(id);
            }
        } else if (e.type == ElementType::Inductor) {
            if (is_selected) {
                // L, bit=0: выделение -> разрыв
                transformed.deactivate_element(id);
            } else {
                // L, bit=1: нейтрализация -> провод
                transformed.deactivate_element(id);
                transformed.merge_vertices(u, v);
            }
        }

        ++reactive_index;
    }

    if (reactive_index != reactive_count) {
        throw std::runtime_error("Reactive count mismatch in build_by_mask()");
    }

    // Фаза 2: строим резистивный Graph из оставшихся активных резисторов.
    Graph graph(transformed.n);

    for (const auto& e : transformed.elements) {
        if (!e.is_active) continue;

        if (e.type != ElementType::Resistor) {
            continue;
        }

        // Одиночный резистор: Y = 1, Z = R.
        graph.add_edge(e.u, e.v, "1", e.name);
    }

    return GeneratedScheme{graph, multiplier, p_degree};
}

void RLCScheme::print() const {
    std::cout << "RLC scheme\n";
    std::cout << "Vertices: " << n << "\n";
    std::cout << "Elements: " << elements.size() << "\n";
    std::cout << "Reactive elements: " << reactive_count << "\n";

    for (size_t id = 0; id < elements.size(); ++id) {
        const auto& e = elements[id];

        std::cout << id + 1 << ": "
                  << type_to_string(e.type) << " "
                  << e.name << " "
                  << e.u + 1 << " "
                  << e.v + 1
                  << (e.is_active ? " active" : " inactive")
                  << "\n";
    }
}