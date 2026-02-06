#pragma once

#include <vector>
#include <unordered_map>
#include "../manager.hpp"
#include "em_base.hpp"
#include "em_types.hpp"
#include "em_utils.hpp"

namespace shrg::vi {

// Utility functions for digamma calculation
namespace detail {
double digamma(double x);
}

class VariationalInference : public em::EMBase {
public:
    VariationalInference(RuleVector &shrg_rules,
                        std::vector<EdsGraph> &graphs,
                        Context *context,
                        double threshold,
                        double alpha = 1.0);  // Dirichlet prior concentration

    void run() override;

protected:
    double computeVariationalInside(ChartItem* root);
    void computeVariationalOutside(ChartItem* root);
    void computeExpectedCount(ChartItem* root, double pw) override;
    void updateEM();
    void traverseForELBO(ChartItem* root, double& expected_ll);
    void verifyELBOIncrease(double new_elbo, double old_elbo);
    double computeExpectedLogProb(SHRG* rule);
    double computePriorContribution();
    bool converged() const override;

private:
    double alpha_;  // Dirichlet prior concentration parameter
    double elbo_;   // Evidence Lower BOund
    double prev_elbo_;
    size_t total_examples_;
    LabelToRule rule_dict_;

    // Variational parameters
    std::unordered_map<SHRG*, double> gamma_;  // Dirichlet parameters

    LabelToRule getRuleDict();
};

} // namespace shrg::vi