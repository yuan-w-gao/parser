/**
 * @file derivation_entropy.cpp
 * @brief Compute derivation entropy: H = log Z - sum_n γ(n) log w(n)
 *
 * Derivation entropy measures uncertainty over the distribution of derivations.
 *
 * Formula derivation:
 *   H = -sum_d p(d) log p(d)
 *     = log Z - sum_n γ(n) log w(n)
 *
 * where:
 *   - Z = partition function (sum of all derivation weights)
 *   - γ(n) = E[c_n(d)] = expected count of item n under p(d)
 *   - w(n) = weight of item n
 *   - γ(n) = α(n) * β(v) / Z  (computed via inside-outside)
 *
 * Higher entropy indicates more ambiguity in the derivation distribution.
 */

#include "ambiguity_metrics/ambiguity_metrics.hpp"
#include "graph_parser/parser_chart_item.hpp"

#include <unordered_set>
#include <cmath>
#include <iostream>

namespace lexcxg {

namespace {

/**
 * @brief Recursive helper for computing sum_n γ(n) * log w(n)
 *
 * Traverses the forest, computing the weighted log-weight sum.
 * Uses memoization to avoid recomputation.
 *
 * @param node Current chart item (OR-node entry point)
 * @param log_Z Log partition function
 * @param sum_gamma_log_w Accumulator for sum of γ(n) * log w(n)
 * @param visited Set of visited items to avoid recomputation
 */
void ComputeGammaLogWeightSum(
    shrg::ChartItem* node,
    double log_Z,
    double& sum_gamma_log_w,
    std::unordered_set<shrg::ChartItem*>& visited
) {
    if (!node || visited.count(node)) {
        return;
    }

    shrg::ChartItem* ptr = node;
    shrg::ChartItem* start = node;

    // Iterate through all alternatives in the next_ptr cycle (OR-node)
    do {
        if (visited.count(ptr)) {
            ptr = ptr->next_ptr;
            continue;
        }
        visited.insert(ptr);

        // Compute γ(n) = α(n) * β(v) / Z for this item
        // In log space: log γ(n) = log_inside + log_outside - log_Z
        double log_inside = ptr->log_inside_prob;
        double log_outside = ptr->log_outside_prob;

        if (IsValidProb(log_inside) && IsValidProb(log_outside)) {
            double log_gamma = log_inside + log_outside - log_Z;
            double gamma = std::exp(log_gamma);

            // Get log weight of this item: log w(n)
            double log_w = 0.0;  // default: w = 1, log w = 0
            if (ptr->rule_ptr) {
                log_w = ptr->rule_ptr->log_rule_weight;
            }

            // Add γ(n) * log w(n) to the sum
            if (gamma > 0 && std::isfinite(gamma) && std::isfinite(log_w)) {
                sum_gamma_log_w += gamma * log_w;
            }
        }

        // Recurse to children (child OR-nodes)
        for (shrg::ChartItem* child : ptr->children) {
            ComputeGammaLogWeightSum(child, log_Z, sum_gamma_log_w, visited);
        }

        ptr = ptr->next_ptr;
    } while (ptr && ptr != start);
}

} // anonymous namespace

double ComputeDerivationEntropy(shrg::ChartItem* root, double log_partition, bool debug) {
    if (!root) {
        return 0.0;
    }

    // log_Z = log partition function = log(sum of all derivation weights)
    // This equals the inside probability at the root OR-node
    double log_Z = log_partition;
    if (!IsValidProb(log_Z)) {
        log_Z = root->log_inside_prob;
    }

    if (debug) {
        std::cerr << "[DEBUG] Derivation Entropy Computation\n";
        std::cerr << "[DEBUG] log_partition=" << log_partition
                  << " root->log_inside=" << root->log_inside_prob
                  << " log_Z=" << log_Z << "\n";
    }

    if (!IsValidProb(log_Z)) {
        if (debug) std::cerr << "[DEBUG] Invalid log_Z, returning 0\n";
        return 0.0;  // Can't compute entropy without valid partition function
    }

    // Compute sum_n γ(n) * log w(n)
    double sum_gamma_log_w = 0.0;
    std::unordered_set<shrg::ChartItem*> visited;

    ComputeGammaLogWeightSum(root, log_Z, sum_gamma_log_w, visited);

    // H = log Z - sum_n γ(n) * log w(n)
    double entropy = log_Z - sum_gamma_log_w;

    if (debug) {
        std::cerr << "[DEBUG] log_Z=" << log_Z
                  << " sum_gamma_log_w=" << sum_gamma_log_w
                  << " entropy=" << entropy
                  << " visited=" << visited.size() << "\n";
    }

    // Entropy should be non-negative (may be slightly negative due to numerical errors)
    return std::max(0.0, entropy);
}

double ComputeDerivationEntropy(shrg::ChartItem* root, double log_partition) {
    return ComputeDerivationEntropy(root, log_partition, false);
}

} // namespace lexcxg
