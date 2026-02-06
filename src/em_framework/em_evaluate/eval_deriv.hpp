//
// Created by Yuan Gao on 28/01/2025.
//

#ifndef EVAL_DERIV_HPP
#define EVAL_DERIV_HPP


#endif //EVAL_DERIV_HPP
#include "../em_base.hpp"
#include "../em_utils.hpp"
#include "../em_types.hpp"
#include "../find_derivations.hpp"

#include <map>
#include <vector>

namespace shrg::em {
class EVALUATE_DERIVATION {
public:
    enum Deriv_Type {
        EM_Greedy_deriv,
        EM_Inside_deriv,
        Count_Greedy_deriv,
        Count_Inside_deriv,
        Baseline_Sample_deriv,
    };
    using graphID_to_ruleIndex = std::map<std::string, std::vector<int>>;
    std::vector<int> get_derivation(ChartItem *root, Deriv_Type type);
    graphID_to_ruleIndex get_derivation_all(Deriv_Type type);
    std::vector<std::vector<int>> get_derivation_all(std::vector<ChartItem*> roots, Deriv_Type type);
    using derivation_vector = std::vector<std::vector<int>>;
    std::pair<derivation_vector, derivation_vector> get_count_derivs(std::vector<ChartItem*> roots);
    std::vector<int> get_rule_indices(ChartItem *root, Deriv_Type type);

    explicit EVALUATE_DERIVATION(EMBase *em);

  private:
    EMBase *em;
    Context *context;
};
}