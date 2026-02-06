#include "variational_inference.hpp"
#include <cmath>
#include <iostream>
#include <queue>

namespace shrg::vi {

namespace detail {
    // Implementation of the digamma function using a series expansion
    // Based on the implementation from scipy
    double digamma(double x) {
        if (x <= 0) {
            return std::numeric_limits<double>::quiet_NaN();
        }

        double result = 0;
        double x_initial = x;

        // Use reflection formula for small x
        if (x <= 1) {
            result -= 1/x;
            x += 1;
        }

        // Series expansion for x >= 1
        if (x < 12) {
            double y = x - 1;
            while (y < 12) {
                result -= 1/y;
                y += 1;
            }
            x = y;
        }

        // Asymptotic expansion for x >= 12
        if (x >= 12) {
            double r = 1/x;
            result = std::log(x) - 0.5*r;
            r *= r;

            // Add rational approximation terms
            result -= r * (1.0/12 - r * (1.0/120 - r * (1.0/252)));
        }

        return result;
    }
}

VariationalInference::VariationalInference(RuleVector &shrg_rules,
                                         std::vector<EdsGraph> &graphs,
                                         Context *context,
                                         double threshold,
                                         double alpha)
    : EMBase(shrg_rules, graphs, context, threshold),
      alpha_(alpha) {
    prev_elbo_ = 0.0;
    rule_dict_ = getRuleDict();
    total_examples_ = graphs.size();

    // Initialize variational parameters
    for (auto rule : shrg_rules) {
        // Initialize gamma (variational Dirichlet parameters)
        gamma_[rule] = alpha_;
    }
}

void VariationalInference::traverseForELBO(ChartItem* root, double& expected_ll) {
    if (!root || root->count_visited_status != VISITED) {
        return;
    }

    ChartItem* ptr = root;
    do {
        // Add contribution of this rule using log-space arithmetic
        if (ptr->log_sent_rule_count != ChartItem::log_zero) {
            // Note: Both log_sent_rule_count and expected_log_prob are already negative
            double log_contribution = ptr->log_sent_rule_count +
                                    computeExpectedLogProb(ptr->rule_ptr);
            expected_ll = addLogs(expected_ll, log_contribution);
        }

        // Recursively process children
        for (ChartItem* child : ptr->children) {
            traverseForELBO(child, expected_ll);
        }

        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void VariationalInference::verifyELBOIncrease(double new_elbo, double old_elbo) {
    const double tolerance = -1e-10;  // Allow for small numerical errors
    if (new_elbo < old_elbo + tolerance) {
        std::cerr << "Warning: ELBO decreased from " << old_elbo
                  << " to " << new_elbo
                  << " (difference: " << new_elbo - old_elbo << ")\n";
    }
}

void VariationalInference::run() {
    std::cout << "Running Variational Inference...\n";
    int iteration = 0;
    elbo_ = -std::numeric_limits<double>::infinity();  // Initialize to negative infinity


    do {
        prev_elbo_ = elbo_;
        elbo_ = 0.0;
        clearRuleCount();

        // E-step: Update variational distribution over latent variables (parses)
        for (size_t i = 0; i < graphs.size(); i++) {
            if (i % 200 == 0) {
                std::cout << "Processing graph " << i << "\n";
            }

            EdsGraph& graph = graphs[i];
            auto code = context->Parse(graph);

            if (code == ParserError::kNone) {
                ChartItem* root = context->parser->Result();
                addParentPointerOptimized(root, 0);
                addRulePointer(root);

                // Compute expected counts and likelihood under current variational distribution
                double pw = computeVariationalInside(root);
                computeVariationalOutside(root);
                computeExpectedCount(root, pw);

                // Add expected log likelihood to ELBO
                double expected_ll = 0.0;
                traverseForELBO(root, expected_ll);
                elbo_ += expected_ll;
            }
        }

        // M-step: Update variational parameters
        updateEM();

        // Add Dirichlet prior contribution to ELBO
        elbo_ += computePriorContribution();

        // Verify ELBO is increasing
        verifyELBOIncrease(elbo_, prev_elbo_);

        std::cout << "Iteration: " << iteration
                  << "\nELBO: " << elbo_
                  << "\nChange in ELBO: " << elbo_ - prev_elbo_ << "\n\n";

        iteration++;
    } while (!converged());
}

double VariationalInference::computeVariationalInside(ChartItem* root) {
    if (root->inside_visited_status == VISITED) {
        return root->log_inside_prob;
    }

    ChartItem* ptr = root;
    double log_inside = ChartItem::log_zero;

    do {
        // Use expected log probability under variational distribution
        double curr_log_inside = computeExpectedLogProb(ptr->rule_ptr);

        double log_children = 0.0;
        for (ChartItem* child : ptr->children) {
            log_children += computeVariationalInside(child);
        }

        curr_log_inside += log_children;
        log_inside = addLogs(log_inside, curr_log_inside);

        ptr = ptr->next_ptr;
    } while (ptr != root);

    // Store results
    do {
        ptr->log_inside_prob = log_inside;
        ptr->inside_visited_status = VISITED;
        ptr = ptr->next_ptr;
    } while (ptr != root);

    return log_inside;
}

void VariationalInference::computeVariationalOutside(ChartItem* root) {
    NodeLevelPQ pq;
    root->log_outside_prob = 0.0;
    pq.push(root);

    while (!pq.empty()) {
        ChartItem* node = pq.top();
        pq.pop();

        if (!node->parents_sib.empty()) {
            double log_outside = ChartItem::log_zero;

            for (auto& parent_sib : node->parents_sib) {
                ChartItem* parent = getParent(parent_sib);
                std::vector<ChartItem*> siblings = getSiblings(parent_sib);

                double curr_log_outside = computeExpectedLogProb(parent->rule_ptr);
                curr_log_outside += parent->log_outside_prob;

                for (auto sib : siblings) {
                    curr_log_outside += sib->log_inside_prob;
                }

                log_outside = addLogs(log_outside, curr_log_outside);
            }

            node->log_outside_prob = log_outside;
            node->outside_visited_status = VISITED;
        }

        for (ChartItem* child : node->children) {
            pq.push(child);
        }
    }
}

void VariationalInference::computeExpectedCount(ChartItem* root, double pw) {
    if (root->count_visited_status == VISITED) {
        return;
    }

    ChartItem* ptr = root;
    do {
        double curr_log_count = computeExpectedLogProb(ptr->rule_ptr);
        curr_log_count += ptr->log_outside_prob;
        curr_log_count -= pw;

        for (ChartItem* child : ptr->children) {
            curr_log_count += child->log_inside_prob;
        }

        ptr->log_sent_rule_count = curr_log_count;
        ptr->rule_ptr->log_count = addLogs(ptr->rule_ptr->log_count, curr_log_count);

        ptr->count_visited_status = VISITED;
        for (ChartItem* child : ptr->children) {
            computeExpectedCount(child, pw);
        }
        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void VariationalInference::updateEM() {
    // Step size for parameter updates (can be tuned)
    const double step_size = 0.1;

    // Store old parameters
    std::unordered_map<SHRG*, double> old_gamma = gamma_;

    // Group rules by label hash and update parameters
    for (auto& pair : rule_dict_) {
        LabelHash label = pair.first;

        // Compute sufficient statistics
        double log_total_count = ChartItem::log_zero;
        for (auto rule : pair.second) {
            if (rule->log_count != ChartItem::log_zero) {
                log_total_count = addLogs(log_total_count, rule->log_count);
            }
        }

        // Update gamma parameters with natural gradient
        for (auto rule : pair.second) {
            double expected_count = alpha_;  // Start with prior
            if (rule->log_count != ChartItem::log_zero) {
                expected_count += std::exp(rule->log_count);
            }

            // Natural gradient update with step size
            double natural_grad = expected_count - gamma_[rule];
            gamma_[rule] = old_gamma[rule] + step_size * natural_grad;

            // Ensure gamma stays positive
            gamma_[rule] = std::max(gamma_[rule], 1e-10);
        }
    }

    // Normalize gamma parameters within each label group
    for (auto& pair : rule_dict_) {
        double sum_gamma = 0.0;
        for (auto rule : pair.second) {
            sum_gamma += gamma_[rule];
        }

        // Normalize to maintain valid Dirichlet parameters
        for (auto rule : pair.second) {
            gamma_[rule] *= pair.second.size() / sum_gamma;
        }
    }
}

double VariationalInference::computeExpectedLogProb(SHRG* rule) {
    // Compute E[log p(rule)] under variational distribution
    // For Dirichlet, this is digamma(gamma_k) - digamma(sum(gamma))
    double sum_gamma = 0.0;
    for (auto other_rule : rule_dict_[rule->label_hash]) {
        sum_gamma += gamma_[other_rule];
    }

    // Return negative log probability
    return -(detail::digamma(gamma_[rule]) - detail::digamma(sum_gamma));
}

double VariationalInference::computePriorContribution() {
    // Compute KL divergence between variational posterior and prior
    double kl_div = 0.0;

    for (const auto& pair : rule_dict_) {
        double sum_gamma = 0.0;
        for (auto rule : pair.second) {
            sum_gamma += gamma_[rule];

            // Add terms for each rule
            kl_div += (gamma_[rule] - alpha_) *
                     (detail::digamma(gamma_[rule]) - detail::digamma(sum_gamma));
            kl_div += std::lgamma(gamma_[rule]) - std::lgamma(alpha_);
        }

        kl_div += std::lgamma(pair.second.size() * alpha_) -
                 std::lgamma(sum_gamma);
    }

    return -kl_div;  // Return negative KL divergence as it's subtracted from ELBO
}

bool VariationalInference::converged() const {
    return std::abs(elbo_ - prev_elbo_) <= threshold;
}

LabelToRule VariationalInference::getRuleDict() {
    LabelToRule dict;
    for (auto rule : shrg_rules) {
        dict[rule->label_hash].push_back(rule);
    }

    // Remove duplicates
    for (auto& pair : dict) {
        std::set<SHRG*> s(pair.second.begin(), pair.second.end());
        pair.second = RuleVector(s.begin(), s.end());
    }
    return dict;
}

} // namespace shrg::vi