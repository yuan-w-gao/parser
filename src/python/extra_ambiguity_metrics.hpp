#ifndef EXTRA_AMBIGUITY_METRICS_HPP
#define EXTRA_AMBIGUITY_METRICS_HPP

#include <limits>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../ambiguity_metrics/ambiguity_metrics.hpp"
#include "../manager.hpp"

namespace shrg {

/**
 * @brief Compute derivation entropy for the current parse result
 *
 * This wraps the C++ ComputeDerivationEntropy function for Python.
 * Requires that inside-outside has been computed (via EM or explicit call).
 *
 * @param context The parsing context after parse() and inside-outside computation
 * @return Entropy in nats, or 0 if no valid parse
 */
inline double Context_ComputeEntropy(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    if (!root) {
        return 0.0;
    }
    // Use the inside probability at root as the partition function
    double log_partition = root->log_inside_prob;
    return lexcxg::ComputeDerivationEntropy(root, log_partition);
}

/**
 * @brief Compute entropy with explicit partition function
 */
inline double Context_ComputeEntropyWithPartition(Context& context, double log_partition) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    if (!root) {
        return 0.0;
    }
    return lexcxg::ComputeDerivationEntropy(root, log_partition);
}

/**
 * @brief Compute expected derivation count
 */
inline double Context_ComputeDerivationCount(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    if (!root) {
        return 0.0;
    }
    return lexcxg::ComputeExpectedDerivationCount(root);
}

/**
 * @brief Compute log of derivation count (for overflow safety)
 */
inline double Context_ComputeLogDerivationCount(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    if (!root) {
        return -std::numeric_limits<double>::infinity();
    }
    return lexcxg::ComputeLogDerivationCount(root);
}

/**
 * @brief Compute forest stats (nodes, edges, depth, branching)
 */
inline pybind11::dict Context_ComputeForestStats(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    pybind11::dict result;

    if (!root) {
        result["num_nodes"] = 0;
        result["num_edges"] = 0;
        result["max_depth"] = 0;
        result["avg_branching"] = 0.0;
        result["complexity"] = 0.0;
        return result;
    }

    lexcxg::ForestStats stats = lexcxg::ComputeForestComplexity(root);
    result["num_nodes"] = stats.num_nodes;
    result["num_edges"] = stats.num_edges;
    result["max_depth"] = stats.max_depth;
    result["avg_branching"] = stats.avg_branching;
    result["complexity"] = stats.complexity;
    return result;
}

/**
 * @brief Compute all ambiguity metrics at once
 */
inline pybind11::dict Context_ComputeAllMetrics(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    pybind11::dict result;

    if (!root) {
        result["entropy"] = 0.0;
        result["expected_count"] = 0.0;
        result["num_nodes"] = 0;
        result["num_edges"] = 0;
        result["max_depth"] = 0;
        result["avg_branching"] = 0.0;
        result["complexity"] = 0.0;
        result["num_alternatives"] = 0;
        result["has_valid_probs"] = false;
        return result;
    }

    double log_partition = root->log_inside_prob;
    lexcxg::AmbiguityMetrics metrics = lexcxg::ComputeAllMetrics(root, log_partition);

    result["entropy"] = metrics.entropy;
    result["expected_count"] = metrics.expected_count;
    result["num_nodes"] = metrics.forest_stats.num_nodes;
    result["num_edges"] = metrics.forest_stats.num_edges;
    result["max_depth"] = metrics.forest_stats.max_depth;
    result["avg_branching"] = metrics.forest_stats.avg_branching;
    result["complexity"] = metrics.forest_stats.complexity;
    result["num_alternatives"] = metrics.num_derivation_alternatives;
    result["has_valid_probs"] = metrics.has_valid_probabilities;
    return result;
}

/**
 * @brief Run inside-outside computation on the current parse
 *
 * This must be called before computing entropy if EM hasn't been run.
 * Returns the log partition function (inside probability at root).
 */
double Context_ComputeInsideOutside(Context& context);

/**
 * @brief Get the log inside probability (partition function) at root
 */
inline double Context_GetLogPartition(Context& context) {
    ChartItem* root = const_cast<ChartItem*>(context.Result());
    if (!root) {
        return -std::numeric_limits<double>::infinity();
    }
    return root->log_inside_prob;
}

} // namespace shrg

#endif // EXTRA_AMBIGUITY_METRICS_HPP
