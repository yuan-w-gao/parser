#pragma once

#include <unordered_map>
#include <vector>
#include <limits>
#include <cmath>
#include <map>

#include "../em_base.hpp"

namespace shrg::em{
class EVALUATE_TREE{
  public:
    struct Derivation_Eval{
        using graphID_to_ruleIndex = std::map<std::string, std::vector<int>>;
        graphID_to_ruleIndex em_deriv;
        graphID_to_ruleIndex count_greedy_deriv;
        graphID_to_ruleIndex count_inside_deriv;
    };
    explicit EVALUATE_TREE(EMBase *em);

    Derivation extractBestDerivationGreedy(ChartItem *root);
    Derivation extractBestDerivationGlobal(ChartItem *root);

    void getEMDerivation(std::vector<std::string> graph_id, std::vector<ChartItem*> parsed);


    double compareDerivations(const Derivation& deriv1, const Derivation& deriv2);
    std::vector<int> DerivationToRuleIndex(Derivation &d, const std::string file_name);

  private:
    EMBase *em;
    Context* context;

    int addDerivationNode(Derivation& derivation,
                          ChartItem* item,
                          const SHRG::CFGRule* best_cfg = nullptr);
};
}