/**
 * @file forest_complexity.cpp
 * @brief Compute forest complexity: nodes x branching x depth
 *
 * A composite metric capturing both the size and structure of the derivation forest.
 */

#include "ambiguity_metrics/ambiguity_metrics.hpp"
#include "graph_parser/parser_chart_item.hpp"

#include <unordered_set>
#include <algorithm>
#include <functional>

namespace lexcxg {

namespace {

/**
 * @brief Recursive helper for forest statistics computation
 *
 * Single DFS traversal collecting node count, edge count, and max depth.
 */
void ComputeStatsRecursive(
    shrg::ChartItem* node,
    std::unordered_set<shrg::ChartItem*>& visited,
    ForestStats& stats,
    int depth
) {
    if (!node) {
        return;
    }

    shrg::ChartItem* ptr = node;
    shrg::ChartItem* root = node;

    // Iterate through all alternatives in the next_ptr cycle
    do {
        if (visited.count(ptr)) {
            ptr = ptr->next_ptr;
            continue;
        }

        visited.insert(ptr);
        stats.num_nodes++;
        stats.num_edges += static_cast<int>(ptr->children.size());
        stats.max_depth = std::max(stats.max_depth, depth);

        // Recurse to children
        for (shrg::ChartItem* child : ptr->children) {
            ComputeStatsRecursive(child, visited, stats, depth + 1);
        }

        ptr = ptr->next_ptr;
    } while (ptr && ptr != root);
}

} // anonymous namespace

ForestStats ComputeForestComplexity(shrg::ChartItem* root) {
    ForestStats stats;

    if (!root) {
        return stats;
    }

    std::unordered_set<shrg::ChartItem*> visited;
    ComputeStatsRecursive(root, visited, stats, 0);

    // Compute derived metrics
    if (stats.num_nodes > 0) {
        stats.avg_branching = static_cast<double>(stats.num_edges) / stats.num_nodes;
    }

    // Composite complexity score
    stats.complexity = stats.num_nodes * stats.avg_branching * stats.max_depth;

    return stats;
}

/**
 * @brief Compute all ambiguity metrics at once
 *
 * Combines entropy, expected count, and complexity computation.
 * Note: Entropy requires inside/outside probabilities to have been computed.
 */
AmbiguityMetrics ComputeAllMetrics(shrg::ChartItem* root, double log_partition) {
    AmbiguityMetrics metrics;

    if (!root) {
        return metrics;
    }

    // Compute expected derivation count (no probabilities needed)
    metrics.expected_count = ComputeExpectedDerivationCount(root);

    // Compute entropy (requires valid probabilities)
    if (IsValidProb(log_partition) || IsValidProb(root->log_inside_prob)) {
        metrics.entropy = ComputeDerivationEntropy(root, log_partition, false);
        metrics.has_valid_probabilities = true;
    }

    // Compute forest complexity (no probabilities needed)
    metrics.forest_stats = ComputeForestComplexity(root);

    // Count total alternatives across all nodes
    std::unordered_set<shrg::ChartItem*> visited;
    std::function<void(shrg::ChartItem*)> countAlternatives = [&](shrg::ChartItem* node) {
        if (!node) return;

        shrg::ChartItem* ptr = node;
        shrg::ChartItem* nodeRoot = node;
        int alt_count = 0;

        do {
            if (visited.count(ptr)) {
                ptr = ptr->next_ptr;
                continue;
            }
            visited.insert(ptr);
            alt_count++;

            for (shrg::ChartItem* child : ptr->children) {
                countAlternatives(child);
            }

            ptr = ptr->next_ptr;
        } while (ptr && ptr != nodeRoot);

        if (alt_count > 1) {
            metrics.num_derivation_alternatives += alt_count;
        }
    };

    visited.clear();
    countAlternatives(root);

    return metrics;
}

} // namespace lexcxg
