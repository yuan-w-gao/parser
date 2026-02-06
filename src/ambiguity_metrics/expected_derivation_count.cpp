/**
 * @file expected_derivation_count.cpp
 * @brief Compute expected derivation count via DP on forest structure
 *
 * For each node: E[count] = sum over alternatives of product of child counts
 * This gives the total number of distinct derivation trees in the forest.
 */

#include "ambiguity_metrics/ambiguity_metrics.hpp"
#include "graph_parser/parser_chart_item.hpp"
#include "graph_parser/generator.hpp"

#include <unordered_map>
#include <queue>

namespace lexcxg {

// Visited flag constant for children population
static const int CHILDREN_VISITED = 999;

/**
 * @brief Recursive helper to populate children pointers
 */
void PopulateChildrenRecursive(
    shrg::ChartItem* root,
    shrg::Generator* generator
) {
    if (!root || root->child_visited_status == CHILDREN_VISITED) {
        return;
    }

    shrg::ChartItem* start = root;
    shrg::ChartItem* ptr = start;

    do {
        if (ptr->child_visited_status != CHILDREN_VISITED) {
            const shrg::SHRG* rule = ptr->attrs_ptr->grammar_ptr;

            // Add children based on nonterminal edges in the rule
            ptr->children.reserve(rule->nonterminal_edges.size());
            for (auto edge_ptr : rule->nonterminal_edges) {
                shrg::ChartItem* child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                PopulateChildrenRecursive(child, generator);
            }

            ptr->child_visited_status = CHILDREN_VISITED;
        } else {
            // Already visited, but still recurse to children
            for (auto child : ptr->children) {
                PopulateChildrenRecursive(child, generator);
            }
        }

        ptr = ptr->next_ptr;
    } while (ptr != start);
}

void PopulateChildren(shrg::ChartItem* root, shrg::Generator* generator) {
    if (!root || !generator) return;
    PopulateChildrenRecursive(root, generator);
}

namespace {

/**
 * @brief Recursive helper for derivation count computation
 *
 * Uses dynamic programming with memoization.
 * For a node with alternatives A_1, A_2, ..., A_k:
 *   E[count] = sum_{i=1}^{k} product_{c in children(A_i)} E[count(c)]
 */
double ComputeCountRecursive(
    shrg::ChartItem* node,
    std::unordered_map<shrg::ChartItem*, double>& cache
) {
    if (!node) {
        return 0.0;
    }

    // Use canonical node (lowest address in cycle) as cache key
    shrg::ChartItem* canonical = GetCanonicalNode(node);

    auto it = cache.find(canonical);
    if (it != cache.end()) {
        return it->second;
    }

    double total_count = 0.0;
    shrg::ChartItem* ptr = node;
    shrg::ChartItem* root = node;

    // Iterate through all alternatives in the next_ptr cycle
    do {
        // For this alternative, count = product of children counts
        double alt_count = 1.0;

        if (ptr->children.empty()) {
            // Leaf node: exactly 1 derivation
            alt_count = 1.0;
        } else {
            // Internal node: multiply children counts
            for (shrg::ChartItem* child : ptr->children) {
                double child_count = ComputeCountRecursive(child, cache);
                alt_count *= child_count;

                // Prevent overflow
                if (alt_count > 1e100) {
                    alt_count = 1e100;
                }
            }
        }

        total_count += alt_count;

        // Prevent overflow
        if (total_count > 1e100) {
            total_count = 1e100;
        }

        ptr = ptr->next_ptr;
    } while (ptr && ptr != root);

    cache[canonical] = total_count;
    return total_count;
}

} // anonymous namespace

shrg::ChartItem* GetCanonicalNode(shrg::ChartItem* node) {
    if (!node) {
        return nullptr;
    }

    shrg::ChartItem* canonical = node;
    shrg::ChartItem* ptr = node->next_ptr;

    // Find the node with the lowest memory address in the cycle
    while (ptr && ptr != node) {
        if (ptr < canonical) {
            canonical = ptr;
        }
        ptr = ptr->next_ptr;
    }

    return canonical;
}

double ComputeExpectedDerivationCount(shrg::ChartItem* root) {
    if (!root) {
        return 0.0;
    }

    std::unordered_map<shrg::ChartItem*, double> cache;
    return ComputeCountRecursive(root, cache);
}

namespace {

/**
 * @brief Recursive helper for log-space derivation count computation
 *
 * Uses log-sum-exp to avoid overflow.
 * log(count) = log(sum_i product_j count[child_j])
 *            = log(sum_i exp(sum_j log(count[child_j])))
 */
double ComputeLogCountRecursive(
    shrg::ChartItem* node,
    std::unordered_map<shrg::ChartItem*, double>& cache
) {
    if (!node) {
        return -std::numeric_limits<double>::infinity();
    }

    // Use canonical node (lowest address in cycle) as cache key
    shrg::ChartItem* canonical = GetCanonicalNode(node);

    auto it = cache.find(canonical);
    if (it != cache.end()) {
        return it->second;
    }

    double log_total = -std::numeric_limits<double>::infinity();
    shrg::ChartItem* ptr = node;
    shrg::ChartItem* root = node;

    // Iterate through all alternatives in the next_ptr cycle
    do {
        // For this alternative, log_count = sum of log(children counts)
        double log_alt_count = 0.0;  // log(1) = 0

        if (!ptr->children.empty()) {
            // Internal node: sum log counts of children
            for (shrg::ChartItem* child : ptr->children) {
                double log_child_count = ComputeLogCountRecursive(child, cache);
                log_alt_count += log_child_count;
            }
        }

        // log_total = log(exp(log_total) + exp(log_alt_count))
        log_total = LogAdd(log_total, log_alt_count);

        ptr = ptr->next_ptr;
    } while (ptr && ptr != root);

    cache[canonical] = log_total;
    return log_total;
}

} // anonymous namespace

double ComputeLogDerivationCount(shrg::ChartItem* root) {
    if (!root) {
        return -std::numeric_limits<double>::infinity();
    }

    std::unordered_map<shrg::ChartItem*, double> cache;
    return ComputeLogCountRecursive(root, cache);
}

double CountDerivations(shrg::ChartItem* root, shrg::Generator* generator) {
    if (!root || !generator) {
        return 0.0;
    }

    PopulateChildren(root, generator);
    return ComputeExpectedDerivationCount(root);
}

double CountDerivationsLog(shrg::ChartItem* root, shrg::Generator* generator) {
    if (!root || !generator) {
        return -std::numeric_limits<double>::infinity();
    }

    PopulateChildren(root, generator);
    return ComputeLogDerivationCount(root);
}

} // namespace lexcxg
