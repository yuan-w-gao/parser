//
// Created by Yuan Gao on 28/01/2025.
//
#include "eval_deriv.hpp"

namespace shrg::em {
 EVALUATE_DERIVATION::EVALUATE_DERIVATION(EMBase *em) {
     context = em->getContext();
 }

std::vector<int> DerivToIndexVector(Derivation &deriv) {
     std::vector<int> inds;

     for (auto node : deriv) {
         inds.push_back(node.item_ptr->shrg_index);
     }
     return inds;
 }

std::vector<int> EVALUATE_DERIVATION::get_derivation(ChartItem *root, Deriv_Type type) {
     if (type == EM_Greedy_deriv) {
         Derivation d = FindBestDerivation_EMGreedy(root);
         return DerivToIndexVector(d);
     }
     if (type == EM_Inside_deriv) {
         Derivation d = FindBestDerivation_EMInside(root);
         return DerivToIndexVector(d);
     }
     if (type == Count_Greedy_deriv) {
         Derivation d = FindBestDerivation_CountGreedy(root);
         return DerivToIndexVector(d);
     }
     if (type == Count_Inside_deriv) {
         Derivation d = FindBestDerivation_CountInside(root);
         return DerivToIndexVector(d);
     }
 }

std::vector<int> EVALUATE_DERIVATION::get_rule_indices(ChartItem *root, Deriv_Type type) {
     if (type == EM_Greedy_deriv) {
         return ExtractRuleIndices_EMGreedy(root);
     }
     if (type == EM_Inside_deriv) {
         return ExtractRuleIndices_EMInside(root);
     }
     if (type == Count_Greedy_deriv) {
         return ExtractRuleIndices_CountGreedy(root);
     }
     if (type == Count_Inside_deriv) {
         return ExtractRuleIndices_CountInside(root);
     }
     if (type == Baseline_Sample_deriv) {
         return ExtractRuleIndices_sampled(root);
     }
 }

EVALUATE_DERIVATION::graphID_to_ruleIndex
 EVALUATE_DERIVATION::get_derivation_all(Deriv_Type type) {
     EVALUATE_DERIVATION::graphID_to_ruleIndex derivs;
     std::vector<EdsGraph> graphs = em->getGraphs();

     for (auto graph:graphs) {
         std::string graph_id = graph.sentence_id;
         auto code = context->Parse(graph);
         if (code == ParserError::kNone) {
             ChartItem *root = context->parser->Result();
             em->addParentPointerOptimized(root, 0);
             em->addRulePointer(root);
             std::vector<int> inds = get_derivation(root, type);
             derivs[graph_id] = inds;
         }
     }
     return derivs;
 }

std::vector<std::vector<int>> EVALUATE_DERIVATION::get_derivation_all(std::vector<ChartItem *> roots, Deriv_Type type) {
     std::vector<std::vector<int>> derivs;

     for (auto root:roots) {
        std::vector<int> inds = get_derivation(root, type);
         derivs.push_back(inds);
     }
     return derivs;
 }
std::pair<EVALUATE_DERIVATION::derivation_vector, EVALUATE_DERIVATION::derivation_vector> EVALUATE_DERIVATION::get_count_derivs(std::vector<ChartItem*> roots) {
     EVALUATE_DERIVATION::derivation_vector greedy, inside;

     for (auto root: roots) {
         std::vector<int> g = get_derivation(root, EVALUATE_DERIVATION::Count_Greedy_deriv);
         std::vector<int> i = get_derivation(root, EVALUATE_DERIVATION::Count_Inside_deriv);

         greedy.push_back(g);
         inside.push_back(i);
     }
     return {greedy, inside};
 }



}