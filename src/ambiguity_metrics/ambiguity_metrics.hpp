#ifndef LEXCXG_AMBIGUITY_METRICS_HPP
#define LEXCXG_AMBIGUITY_METRICS_HPP

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

// Forward declarations - we'll use these with the parser's types
// when linked, or with mock structures for testing
namespace shrg {
    class ChartItem;
    class Generator;
}

namespace lexcxg {

/**
 * @brief Results from forest complexity computation
 */
struct ForestStats {
    int num_nodes = 0;          // Number of unique nodes in forest
    int num_edges = 0;          // Total edges (sum of children counts)
    int max_depth = 0;          // Maximum depth from root to leaf
    double avg_branching = 0.0; // Average branching factor
    double complexity = 0.0;    // Composite complexity score
};

/**
 * @brief Results from all ambiguity metrics
 */
struct AmbiguityMetrics {
    double entropy = 0.0;             // Derivation entropy
    double expected_count = 0.0;      // Expected derivation count
    ForestStats forest_stats;         // Forest complexity stats

    // Additional diagnostic info
    int num_derivation_alternatives = 0;  // Total alternatives across all nodes
    bool has_valid_probabilities = false; // Whether inside/outside probs are valid
};

/**
 * @brief Compute derivation entropy: H = -sum p(d) log p(d)
 *
 * Uses inside-outside probabilities from EM to compute the entropy
 * of the derivation distribution. Higher entropy = more ambiguity.
 *
 * @param root Root of the derivation forest
 * @param log_partition Log of the partition function (total inside at root)
 * @return Entropy in nats (natural log)
 */
double ComputeDerivationEntropy(::shrg::ChartItem* root, double log_partition);
double ComputeDerivationEntropy(::shrg::ChartItem* root, double log_partition, bool debug);

/**
 * @brief Compute expected derivation count via DP on forest structure
 *
 * For each node: E[count] = sum over alternatives of product of child counts
 *
 * IMPORTANT: Requires children pointers to be populated first.
 * Use PopulateChildren() or CountDerivations() which handles this automatically.
 *
 * @param root Root of the derivation forest
 * @return Expected number of derivation trees
 */
double ComputeExpectedDerivationCount(::shrg::ChartItem* root);

/**
 * @brief Compute log of derivation count to avoid overflow
 *
 * Same as ComputeExpectedDerivationCount but in log space.
 * Use this for highly ambiguous forests where count may overflow double.
 *
 * @param root Root of the derivation forest
 * @return Log of the number of derivation trees
 */
double ComputeLogDerivationCount(::shrg::ChartItem* root);

/**
 * @brief Compute forest complexity: nodes x branching x depth
 *
 * @param root Root of the derivation forest
 * @return ForestStats containing all complexity measures
 */
ForestStats ComputeForestComplexity(::shrg::ChartItem* root);

/**
 * @brief Compute all ambiguity metrics at once
 *
 * More efficient than calling each function separately as it
 * only traverses the forest once where possible.
 *
 * @param root Root of the derivation forest
 * @param log_partition Log of the partition function
 * @return AmbiguityMetrics containing all computed values
 */
AmbiguityMetrics ComputeAllMetrics(::shrg::ChartItem* root, double log_partition);

// =====================================================================
// Children population (required before counting)
// =====================================================================

/**
 * @brief Populate children pointers in the derivation forest
 *
 * This is required before calling ComputeExpectedDerivationCount or
 * ComputeLogDerivationCount. This function is much faster than
 * addParentPointerOptimized because it doesn't compute parents_sib.
 *
 * @param root Root of the derivation forest
 * @param generator Generator instance from parser (for FindChartItemByEdge)
 */
void PopulateChildren(::shrg::ChartItem* root, ::shrg::Generator* generator);

/**
 * @brief Count derivations in one call (populates children + counts)
 *
 * Convenience function that populates children and computes count.
 * More efficient than addParentPointerOptimized when you only need the count.
 *
 * @param root Root of the derivation forest
 * @param generator Generator instance from parser
 * @return Number of derivation trees (capped at 1e100)
 */
double CountDerivations(::shrg::ChartItem* root, ::shrg::Generator* generator);

/**
 * @brief Count derivations in log space (populates children + counts)
 *
 * Same as CountDerivations but returns log(count) to avoid overflow.
 *
 * @param root Root of the derivation forest
 * @param generator Generator instance from parser
 * @return Log of number of derivation trees
 */
double CountDerivationsLog(::shrg::ChartItem* root, ::shrg::Generator* generator);

// =====================================================================
// Helper functions
// =====================================================================

/**
 * @brief Get canonical representative for a next_ptr cycle
 *
 * Returns the node with the lowest memory address in the cycle,
 * used as a unique identifier for memoization.
 */
::shrg::ChartItem* GetCanonicalNode(::shrg::ChartItem* node);

/**
 * @brief Safe log-add operation: log(exp(a) + exp(b))
 */
inline double LogAdd(double a, double b) {
    if (a == -std::numeric_limits<double>::infinity()) return b;
    if (b == -std::numeric_limits<double>::infinity()) return a;
    if (a > b) {
        return a + std::log1p(std::exp(b - a));
    } else {
        return b + std::log1p(std::exp(a - b));
    }
}

/**
 * @brief Check if a probability is valid (not NaN or Inf)
 */
inline bool IsValidProb(double log_prob) {
    return std::isfinite(log_prob);
}

} // namespace lexcxg

#endif // LEXCXG_AMBIGUITY_METRICS_HPP
