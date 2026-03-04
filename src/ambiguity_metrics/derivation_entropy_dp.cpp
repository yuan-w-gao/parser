/**
 * @file derivation_entropy_dp.cpp
 * @brief Compute derivation entropy using bottom-up DP (no outside pass needed)
 *
 * This implements the recursive entropy formula:
 *   H(v) = log Z(v) - sum_a r(a|v) log w(a) + sum_a r(a|v) sum_c H(c)
 *
 * where:
 *   - v is an OR-node (next_ptr cycle = equivalence class)
 *   - a iterates over alternatives (AND-nodes) in the cycle
 *   - r(a|v) = w(a) / Z(v) is the local posterior responsibility
 *   - w(a) = p(rule_a) * prod_c Z(c) is the weight of alternative a
 *   - H(c) is the entropy of child OR-node c
 *
 * Key insight: entropy decomposes via chain rule:
 *   H(v) = H(local choice) + E[H(children)]
 *
 * This formulation:
 *   - Requires only ONE bottom-up pass (no outside computation)
 *   - Naturally handles sharing via memoization on OR-nodes
 *   - Is numerically stable (avoids subtracting large log terms)
 *
 * Mathematical derivation:
 *   H(v) = -sum_d p(d|v) log p(d|v)
 *        = -sum_a r(a|v) log r(a|v) + sum_a r(a|v) sum_c H(c)
 *        = log Z(v) - sum_a r(a|v) log w(a) + sum_a r(a|v) sum_c H(c)
 */

#include "ambiguity_metrics/ambiguity_metrics.hpp"
#include "graph_parser/parser_chart_item.hpp"

#include <unordered_map>
#include <vector>
#include <cmath>
#include <limits>
#include <iostream>

namespace lexcxg {

namespace {

/**
 * @brief Result of computing entropy and partition function for an OR-node
 */
struct OrNodeResult {
    double log_Z;    // log partition function (inside probability)
    double entropy;  // entropy of derivation distribution rooted here
};

/**
 * @brief Get canonical representative for a next_ptr cycle
 *
 * Returns the node with the lowest memory address in the cycle,
 * used as a unique identifier for memoization.
 */
shrg::ChartItem* GetCycleCanonical(shrg::ChartItem* node) {
    if (!node) return nullptr;

    shrg::ChartItem* canonical = node;
    shrg::ChartItem* ptr = node->next_ptr;

    while (ptr && ptr != node) {
        if (ptr < canonical) {
            canonical = ptr;
        }
        ptr = ptr->next_ptr;
    }

    return canonical;
}

/**
 * @brief Compute entropy and partition function for an OR-node (bottom-up DP)
 *
 * This function computes both log Z(v) and H(v) for the OR-node rooted at 'node'.
 * Results are memoized by canonical OR-node pointer.
 *
 * For each alternative a in the OR-node:
 *   log_w(a) = log p(rule_a) + sum_{c in children(a)} log Z(c)
 *
 * Then:
 *   log Z(v) = logsumexp over all alternatives of log_w(a)
 *   r(a|v) = exp(log_w(a) - log Z(v))
 *   H(v) = log Z(v) - sum_a r(a|v) log_w(a) + sum_a r(a|v) sum_c H(c)
 *
 * @param node Entry point to the OR-node (any item in the next_ptr cycle)
 * @param memo Memoization map from canonical OR-node to result
 * @param debug Whether to print debug information
 * @return OrNodeResult containing log_Z and entropy
 */
OrNodeResult ComputeEntropyDP(
    shrg::ChartItem* node,
    std::unordered_map<shrg::ChartItem*, OrNodeResult>& memo,
    bool debug
) {
    if (!node) {
        return {-std::numeric_limits<double>::infinity(), 0.0};
    }

    // Get canonical representative for this OR-node
    shrg::ChartItem* canonical = GetCycleCanonical(node);

    // Check memo
    auto it = memo.find(canonical);
    if (it != memo.end()) {
        return it->second;
    }

    // Collect all alternatives and their weights
    struct Alternative {
        double log_w;                           // log weight of this alternative
        double sum_child_entropy;               // sum of child entropies
        std::vector<shrg::ChartItem*> children; // child OR-nodes
    };
    std::vector<Alternative> alternatives;

    shrg::ChartItem* ptr = node;
    shrg::ChartItem* start = node;

    do {
        Alternative alt;
        alt.sum_child_entropy = 0.0;

        // Get log rule weight
        double log_rule_weight = 0.0;  // default: p = 1, log p = 0
        if (ptr->rule_ptr) {
            log_rule_weight = ptr->rule_ptr->log_rule_weight;
        }

        // Compute log_w(a) = log p(rule) + sum_c log Z(c)
        // and accumulate child entropies
        alt.log_w = log_rule_weight;

        for (shrg::ChartItem* child : ptr->children) {
            OrNodeResult child_result = ComputeEntropyDP(child, memo, debug);
            alt.log_w += child_result.log_Z;
            alt.sum_child_entropy += child_result.entropy;
            alt.children.push_back(child);
        }

        // Only include valid alternatives
        if (std::isfinite(alt.log_w)) {
            alternatives.push_back(alt);
        }

        ptr = ptr->next_ptr;
    } while (ptr && ptr != start);

    // Handle edge cases
    if (alternatives.empty()) {
        OrNodeResult result = {-std::numeric_limits<double>::infinity(), 0.0};
        memo[canonical] = result;
        return result;
    }

    if (alternatives.size() == 1) {
        // Single alternative: no local choice entropy, just propagate child entropy
        OrNodeResult result = {alternatives[0].log_w, alternatives[0].sum_child_entropy};
        memo[canonical] = result;
        return result;
    }

    // Compute log Z(v) = logsumexp of log_w(a)
    // Find max for numerical stability
    double max_log_w = -std::numeric_limits<double>::infinity();
    for (const auto& alt : alternatives) {
        max_log_w = std::max(max_log_w, alt.log_w);
    }

    double sum_exp = 0.0;
    for (const auto& alt : alternatives) {
        sum_exp += std::exp(alt.log_w - max_log_w);
    }
    double log_Z = max_log_w + std::log(sum_exp);

    // Compute responsibilities r(a|v) and entropy terms
    // H(v) = log Z(v) - sum_a r(a|v) log_w(a) + sum_a r(a|v) sum_c H(c)
    double sum_r_log_w = 0.0;
    double sum_r_child_entropy = 0.0;

    for (const auto& alt : alternatives) {
        double r = std::exp(alt.log_w - log_Z);  // responsibility
        sum_r_log_w += r * alt.log_w;
        sum_r_child_entropy += r * alt.sum_child_entropy;
    }

    double entropy = log_Z - sum_r_log_w + sum_r_child_entropy;

    // Entropy should be non-negative (may be slightly negative due to numerical errors)
    entropy = std::max(0.0, entropy);

    if (debug) {
        std::cerr << "[DP] OR-node " << canonical
                  << ": #alts=" << alternatives.size()
                  << " log_Z=" << log_Z
                  << " entropy=" << entropy << "\n";
    }

    OrNodeResult result = {log_Z, entropy};
    memo[canonical] = result;
    return result;
}

} // anonymous namespace

double ComputeDerivationEntropyDP(shrg::ChartItem* root, bool debug) {
    if (!root) {
        return 0.0;
    }

    std::unordered_map<shrg::ChartItem*, OrNodeResult> memo;
    OrNodeResult result = ComputeEntropyDP(root, memo, debug);

    if (debug) {
        std::cerr << "[DP] Final: log_Z=" << result.log_Z
                  << " entropy=" << result.entropy
                  << " #OR-nodes=" << memo.size() << "\n";
    }

    return result.entropy;
}

double ComputeDerivationEntropyDP(shrg::ChartItem* root) {
    return ComputeDerivationEntropyDP(root, false);
}

/**
 * @brief Compute both partition function and entropy via DP
 *
 * This is useful when you need both values, avoiding redundant computation.
 *
 * @param root Root of the derivation forest
 * @param out_log_Z Output: log partition function
 * @param out_entropy Output: derivation entropy
 */
void ComputePartitionAndEntropyDP(
    shrg::ChartItem* root,
    double& out_log_Z,
    double& out_entropy
) {
    if (!root) {
        out_log_Z = -std::numeric_limits<double>::infinity();
        out_entropy = 0.0;
        return;
    }

    std::unordered_map<shrg::ChartItem*, OrNodeResult> memo;
    OrNodeResult result = ComputeEntropyDP(root, memo, false);

    out_log_Z = result.log_Z;
    out_entropy = result.entropy;
}

} // namespace lexcxg
