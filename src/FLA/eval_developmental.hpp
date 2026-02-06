//
// Created by Yuan Gao on 09/12/2024.
//

#ifndef SHRG_GRAPH_PARSER_EVAL_DEVELOPMENTAL_HPP
#define SHRG_GRAPH_PARSER_EVAL_DEVELOPMENTAL_HPP
#include <vector>

#include "../em_framework/em_base.hpp"
#include "eval_single_age.hpp"

class DevelopmentalEvaluator{
  public:
    struct AgeTransition{
        int prev_age;
        int curr_age;
        int shared_rules;
        std::vector<int> new_rules;
        std::vector<int> disappeared_rules;
        double avg_weight_change;
    };

    struct CoreRule{
        int rule_id;
        int first_appearance;
        double avg_weight;
        double stability;
        std::vector<bool> presence;
    };

    static std::vector<AgeTransition> AnalyzeTransitions(
        const std::vector<SingleAgeEvaluator::Metrics>& age_metrics);

    static std::vector<CoreRule> IdentifyCoreRules(
        const std::vector<SingleAgeEvaluator::Metrics>& age_metrics);

    static void WriteResults(const std::vector<AgeTransition>& transitions,
                             const std::vector<CoreRule>& core_rules,
                             const std::string& output_dir);
};



#endif // SHRG_GRAPH_PARSER_EVAL_DEVELOPMENTAL_HPP
