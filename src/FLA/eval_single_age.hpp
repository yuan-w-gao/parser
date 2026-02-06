//
// Created by Yuan Gao on 09/12/2024.
//

#ifndef SHRG_GRAPH_PARSER_EVAL_SINGLE_AGE_HPP
#define SHRG_GRAPH_PARSER_EVAL_SINGLE_AGE_HPP
#include <vector>

#include "../em_framework/em_base.hpp"
#include "../em_framework/eval_utils.hpp"

using namespace shrg;

class SingleAgeEvaluator{
  public:
    struct RuleMetrics {
        int id;
        bool is_lexical;
        int arity;
        std::vector<double> probabilities;
        double baseline_prob;
        std::vector<int> connected_nodes;
    };

    struct Metrics{
        int month;
        std::vector<double> kl_divergence;
        int lexical_rules;
        int structural_rules;
        double avg_derivation_depth;
        std::vector<std::pair<int, double>> significant_weight_changes;
    };
    void LoadProbabilities(std::string &prob_file);
    void CountRuleTypes(std::vector<SHRG> &rules);
    void LoadRules(std::vector<SHRG *> &shrg_rules);
    void TrackSignificantWeightChanges();
    static Metrics Evaluate(const std::vector<SHRG> &rules, const std::string &probability_file, int month);

    static void WriteResults(const Metrics& metrics, const std::string &output_dir);


  private:
    std::vector<RuleMetrics> rule_metrics;
    std::vector<std::vector<double>> rule_probs;
    Metrics metrics;
};


#endif // SHRG_GRAPH_PARSER_EVAL_SINGLE_AGE_HPP
