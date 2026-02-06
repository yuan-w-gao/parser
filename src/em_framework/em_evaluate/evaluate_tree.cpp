#include "evaluate_tree.hpp"

namespace shrg::em{
EVALUATE_TREE::EVALUATE_TREE(EMBase *em):em(em){
    context = em->getContext();
    Derivation_Eval deriv_eval;
}

int EVALUATE_TREE::addDerivationNode(Derivation& derivation,
                                        ChartItem* item,
                                        const SHRG::CFGRule* best_cfg) {
    DerivationNode node;
    node.grammar_ptr = item->rule_ptr;
    node.cfg_ptr = best_cfg ? best_cfg : (item->rule_ptr ? &item->rule_ptr->cfg_rules[0] : nullptr);
    node.item_ptr = item;

    int node_index = derivation.size();
    derivation.push_back(node);
    return node_index;
}

Derivation EVALUATE_TREE::extractBestDerivationGreedy(shrg::ChartItem *root) {
    Derivation derivation;
    if (!root) return derivation;

    int root_index = addDerivationNode(derivation, root);

    for(auto child_ptr :root->children){
        ChartItem *best_child = child_ptr;
        double best_weight = child_ptr->rule_ptr ? child_ptr->rule_ptr->log_rule_weight: -std::numeric_limits<double>::infinity();
        const SHRG::CFGRule* best_cfg = nullptr;

        ChartItem *curr_ptr = child_ptr->next_ptr;
        while(curr_ptr != child_ptr){
            double current_weight = curr_ptr->rule_ptr? curr_ptr->rule_ptr->log_rule_weight:-std::numeric_limits<double>::infinity();
            if(current_weight > best_weight){
                best_weight = current_weight;
                best_child = curr_ptr;
                best_cfg = child_ptr->rule_ptr ? &child_ptr->rule_ptr->cfg_rules[0] : nullptr;
            }

            curr_ptr = curr_ptr->next_ptr;
        }
        Derivation child_derivation = extractBestDerivationGreedy(best_child);
        if (!child_derivation.empty()) {
            derivation[root_index].children.push_back(derivation.size());
            derivation.insert(derivation.end(), child_derivation.begin(), child_derivation.end());
        }
    }
    return derivation;
}

Derivation EVALUATE_TREE::extractBestDerivationGlobal(ChartItem* root) {
    Derivation derivation;
    if (!root) return derivation;

    // Find the alternative with highest inside probability
    ChartItem* best_item = root;
    double best_prob = root->log_inside_prob;
    const SHRG::CFGRule* best_cfg = nullptr;

    ChartItem* current = root->next_ptr;
    while (current != root) {
        if (current->log_inside_prob > best_prob) {
            best_prob = current->log_inside_prob;
            best_item = current;
            best_cfg = current->rule_ptr ? &current->rule_ptr->cfg_rules[0] : nullptr;
        }
        current = current->next_ptr;
    }

    // Create node for best alternative
    int root_index = addDerivationNode(derivation, best_item, best_cfg);

    // Recursively process children
    for (auto child : best_item->children) {
        Derivation child_derivation = extractBestDerivationGlobal(child);
        if (!child_derivation.empty()) {
            derivation[root_index].children.push_back(derivation.size());
            derivation.insert(derivation.end(), child_derivation.begin(), child_derivation.end());
        }
    }

    return derivation;
}
double EVALUATE_TREE::compareDerivations(const Derivation& deriv1, const Derivation& deriv2) {
    if (deriv1.empty() || deriv2.empty()) return 0.0;

    double score = 0.0;
    int total_comparisons = 0;

    auto compare_nodes = [](const DerivationNode& n1, const DerivationNode& n2) -> double {
        double node_score = 0.0;
        int node_comparisons = 0;

        // Compare grammar rules
        if (n1.grammar_ptr && n2.grammar_ptr) {
            if (n1.grammar_ptr->label_hash == n2.grammar_ptr->label_hash) {
                node_score += 1.0;
            }
            node_comparisons++;
        }

        // Compare CFG rules
        if (n1.cfg_ptr && n2.cfg_ptr) {
            if (n1.cfg_ptr->label == n2.cfg_ptr->label) {
                node_score += 1.0;
            }
            node_comparisons++;
        }

        // Compare edge sets
        if (n1.item_ptr && n2.item_ptr) {
            auto edge_intersection = (n1.item_ptr->edge_set & n2.item_ptr->edge_set).count();
            auto edge_union = (n1.item_ptr->edge_set | n2.item_ptr->edge_set).count();
            if (edge_union > 0) {
                node_score += static_cast<double>(edge_intersection) / edge_union;
                node_comparisons++;
            }
        }

        return node_comparisons > 0 ? node_score / node_comparisons : 0.0;
    };

    // Compare each node in the derivations
    for (size_t i = 0; i < std::min(deriv1.size(), deriv2.size()); i++) {
        score += compare_nodes(deriv1[i], deriv2[i]);
        total_comparisons++;

        // Compare structure (number of children)
        if (!deriv1[i].children.empty() || !deriv2[i].children.empty()) {
            double structure_score = deriv1[i].children.size() == deriv2[i].children.size() ? 1.0 : 0.0;
            score += structure_score;
            total_comparisons++;
        }
    }

    return total_comparisons > 0 ? score / total_comparisons : 0.0;
}

std::vector<int> EVALUATE_TREE::DerivationToRuleIndex(Derivation &d, const std::string file_name) {
    std::vector<int> index_inds;
    for (auto node:d) {
        int ind = node.cfg_ptr->shrg_index;
        index_inds.push_back(ind);
    }
}

void EVALUATE_TREE::getEMDerivation(std::vector<std::string> graph_id, std::vector<ChartItem*> parsed) {
    assert(graph_id.size() == parsed.size());
    for (int i = 0; i < graph_id.size(); i++) {

    }
}


}