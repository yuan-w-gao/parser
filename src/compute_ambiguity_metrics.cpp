/**
 * @file compute_ambiguity_metrics.cpp
 * @brief Main entry point for computing ambiguity metrics on derivation forests
 *
 * Usage: compute_ambiguity_metrics <parser_type> <grammar_file> <graph_file> <output_file> <weight_mode>
 *
 * Weight modes:
 *   uniform              - Equal weights per label group
 *   oracle               - MLE weights from training counts (cfg_rule.score)
 *   <weight_history_path> - Load trained weights from file (uses final iteration)
 */

#include "ambiguity_metrics/ambiguity_metrics.hpp"

// Parser includes
#include "manager.hpp"
#include "graph_parser/parser_base.hpp"
#include "em_framework/em.hpp"
#include "em_framework/em_utils.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip>
#include <limits>
#include <cctype>
#include <cerrno>
#include <cmath>

using namespace lexcxg;

// Helper for robust double parsing (handles subnormals, inf, nan)
static inline void trim_inplace(std::string& s) {
    auto issp = [](unsigned char c){ return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back()))  s.pop_back();
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

static inline std::string lower_copy(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static inline bool parse_double_robust(const std::string& raw, double& out) {
    std::string s = raw;
    trim_inplace(s);
    if (s.empty()) return false;

    std::string lo = lower_copy(s);
    if (lo == "inf" || lo == "+inf" || lo == "infinity") {
        out = std::numeric_limits<double>::infinity();
        return true;
    }
    if (lo == "-inf" || lo == "-infinity") {
        out = -std::numeric_limits<double>::infinity();
        return true;
    }
    if (lo == "nan") {
        out = std::numeric_limits<double>::quiet_NaN();
        return true;
    }

    errno = 0;
    char* end = nullptr;
    const double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;
    out = v;
    return true;
}

void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " <parser_type> <grammar_file> <graph_file> <output_file> <weight_mode>\n";
    std::cerr << "\n";
    std::cerr << "Parser types: linear, tree_v1, tree_v2\n";
    std::cerr << "\n";
    std::cerr << "Weight modes:\n";
    std::cerr << "  uniform               - Equal weights per label group\n";
    std::cerr << "  oracle                - MLE weights from training counts\n";
    std::cerr << "  <weight_history_path> - Load trained weights from file\n";
    std::cerr << "\n";
    std::cerr << "Computes ambiguity metrics for each graph:\n";
    std::cerr << "  - Derivation entropy: H = -sum p(d) log p(d)\n";
    std::cerr << "  - Expected derivation count\n";
    std::cerr << "  - Forest complexity: nodes x branching x depth\n";
}

/**
 * @brief Load final weights from weight_history file
 * Format: rule_index,weight_iter0,weight_iter1,...,weight_iterN
 * Returns map from rule_index to final weight (last column)
 */
std::map<int, double> loadFinalWeights(const std::string& filepath) {
    std::map<int, double> weights;
    std::ifstream file(filepath);

    if (!file.is_open()) {
        std::cerr << "Warning: Cannot open weight file: " << filepath << "\n";
        return weights;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string token;

        // First value is rule index
        if (!std::getline(ss, token, ',')) continue;
        int rule_index = std::stoi(token);

        // Read all remaining values, keeping the last one
        double final_weight = 0.0;
        while (std::getline(ss, token, ',')) {
            double val;
            if (parse_double_robust(token, val)) {
                final_weight = val;
            }
        }

        weights[rule_index] = final_weight;
    }

    return weights;
}

/**
 * @brief Apply weights to SHRG rules based on mode
 */
void applyWeights(std::vector<shrg::SHRG*>& shrg_rules,
                  const std::string& weight_mode,
                  shrg::em::EM& em_helper) {

    if (weight_mode == "uniform") {
        // Use uniform weights (already handled by initializeWeights)
        em_helper.initializeWeights();
        std::cout << "Using uniform weights\n";
    }
    else if (weight_mode == "oracle") {
        // Use MLE weights from training counts (cfg_rule.score)
        for (shrg::SHRG* rule : shrg_rules) {
            if (rule && !rule->cfg_rules.empty()) {
                // Use the score from the best CFG rule (already log probability)
                rule->log_rule_weight = rule->best_cfg_ptr->score;
            }
        }
        std::cout << "Using oracle weights (MLE from counts)\n";
    }
    else {
        // Load trained weights from file
        std::map<int, double> weights = loadFinalWeights(weight_mode);
        if (weights.empty()) {
            std::cerr << "Warning: No weights loaded, falling back to uniform\n";
            em_helper.initializeWeights();
        } else {
            int applied = 0;
            for (const auto& [idx, weight] : weights) {
                if (idx >= 0 && idx < static_cast<int>(shrg_rules.size()) && shrg_rules[idx] != nullptr) {
                    shrg_rules[idx]->log_rule_weight = weight;
                    applied++;
                }
            }
            std::cout << "Loaded " << weights.size() << " weights, applied " << applied << " from " << weight_mode << "\n";
            // NOTE: EM weights are already normalized per LHS group, so no additional
            // normalization is needed here. This matches the behavior in evaluate.cpp
            // and get_derivation_edges.cpp which also load weights without normalizing.
        }
    }
}

void writeTsvOutput(
    const std::string& output_file,
    const std::vector<std::pair<int, AmbiguityMetrics>>& results
) {
    std::ofstream out(output_file);
    if (!out) {
        std::cerr << "Error: Cannot open output file: " << output_file << "\n";
        return;
    }

    // Header
    out << "graph_id\texpected_count\tcomplexity\tentropy\tnum_nodes\tnum_edges\tmax_depth\tavg_branching\n";

    double sum_entropy = 0, sum_count = 0, sum_complexity = 0;
    int valid_count = 0;

    for (const auto& [graph_id, metrics] : results) {
        out << graph_id << "\t"
            << std::fixed << std::setprecision(4)
            << metrics.expected_count << "\t"
            << metrics.forest_stats.complexity << "\t"
            << metrics.entropy << "\t"
            << metrics.forest_stats.num_nodes << "\t"
            << metrics.forest_stats.num_edges << "\t"
            << metrics.forest_stats.max_depth << "\t"
            << metrics.forest_stats.avg_branching << "\n";

        sum_entropy += metrics.entropy;
        sum_count += metrics.expected_count;
        sum_complexity += metrics.forest_stats.complexity;
        if (metrics.has_valid_probabilities) valid_count++;
    }

    out.close();

    // Print summary to stdout
    if (!results.empty()) {
        size_t n = results.size();
        std::cout << "n=" << n
                  << " avg_count=" << std::fixed << std::setprecision(2) << (sum_count / n)
                  << " avg_complexity=" << (sum_complexity / n)
                  << " avg_entropy=" << std::setprecision(4) << (sum_entropy / n)
                  << " valid_probs=" << valid_count << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 6) {
        printUsage(argv[0]);
        return 1;
    }

    std::string parser_type = argv[1];
    std::string grammar_file = argv[2];
    std::string graph_file = argv[3];
    std::string output_file = argv[4];
    std::string weight_mode = argv[5];

    std::cout << "=== LexCxG Ambiguity Metrics Computation ===\n";
    std::cout << "Parser type: " << parser_type << "\n";
    std::cout << "Grammar: " << grammar_file << "\n";
    std::cout << "Graphs: " << graph_file << "\n";
    std::cout << "Output: " << output_file << "\n";
    std::cout << "Weight mode: " << weight_mode << "\n\n";

    // Initialize the manager (from parser library)
    shrg::Manager* manager = &shrg::Manager::manager;
    manager->Allocate(1);

    // Load grammars and graphs
    std::cout << "Loading grammars...\n";
    manager->LoadGrammars(grammar_file);

    std::cout << "Loading graphs...\n";
    manager->LoadGraphs(graph_file);

    // Initialize parser context
    shrg::Context* context = manager->contexts[0];
    context->Init(parser_type, false, 100);  // parser_type, verbose=false, max_pool_size=100

    // Create EM instance for tree building and probability computation
    std::vector<shrg::SHRG*> shrg_rules = manager->shrg_rules;
    shrg::em::EM em_helper(shrg_rules, manager->edsgraphs, context, 1.0, "N", 5);

    // Apply weights based on mode
    applyWeights(shrg_rules, weight_mode, em_helper);

    std::vector<std::pair<int, AmbiguityMetrics>> results;

    size_t num_graphs = manager->edsgraphs.size();

    std::cout << "Processing " << num_graphs << " graphs...\n";

    for (size_t i = 0; i < num_graphs; ++i) {
        if (i % 100 == 0) {
            std::cout << "  Processing graph " << i << "/" << num_graphs << "\r" << std::flush;
        }

        // Parse the graph using Context API
        shrg::ParserError error = context->Parse(static_cast<int>(i));

        if (error != shrg::ParserError::kNone) {
            // Failed to parse
            AmbiguityMetrics empty_metrics;
            results.emplace_back(static_cast<int>(i), empty_metrics);
            continue;
        }

        shrg::ChartItem* root = context->parser->Result();
        if (!root) {
            AmbiguityMetrics empty_metrics;
            results.emplace_back(static_cast<int>(i), empty_metrics);
            continue;
        }

        // Build tree structure (children pointers) and compute inside/outside probs
        em_helper.addParentPointerOptimized(root, 0);
        em_helper.addRulePointer(root);
        double log_partition = em_helper.computeInside(root);
        em_helper.computeOutside(root);  // Needed for entropy calculation

        // Compute all ambiguity metrics
        AmbiguityMetrics metrics = ComputeAllMetrics(root, log_partition);

        results.emplace_back(static_cast<int>(i), metrics);
    }

    std::cout << "\nProcessed " << results.size() << " graphs.\n";

    // Write results
    writeTsvOutput(output_file, results);

    return 0;
}
