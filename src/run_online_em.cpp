//
// Created by Yuan Gao on 11/2/22.
//
#include "manager.hpp"

//#include "python/inside_outside.cpp"
#include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"
#include "em_framework/em_base.hpp"
// #include "em_framework/em.hpp"
// #include "em_framework/em_hmm.hpp"
#include "em_framework/em_batch.hpp"
#include "em_framework/em_evaluate/eval_deriv.hpp"
#include "em_framework/em_utils.hpp"
#include "em_framework/em_viterbi.hpp"
#include "python/find_best_derivation.cpp"
#include "test.cpp"
#include "em_framework/em_batch.hpp"
#include "em_framework/em_online.hpp"

#include <set>
#include <iostream>
#include <map>
#include <chrono>

using namespace shrg;


int main(int argc, char *argv[]) {
    clock_t t1,t2, t3, t4;

    auto *manager = &Manager::manager;
    manager->Allocate(1);
    if (argc < 4) {
        LOG_ERROR("Usage: generate <parser_type> <grammar_path> <graph_path>");
        return 1;
    }

    manager->LoadGrammars(argv[2]);
    manager->LoadGraphs(argv[3]);
    auto &context = manager->contexts[0];
    context->Init(argv[1], false , 100 );


    auto graphs = manager->edsgraphs;
    std::vector<SHRG *>shrg_rules = manager->shrg_rules;

    std::cout << "Rules Len: " << shrg_rules.size() << std::endl;
    double threshold = 0.01 * graphs.size();
    std::string out_dir = std::string(argv[4]) + "online_em/";

    shrg::em::OnlineEM model = shrg::em::OnlineEM(shrg_rules, graphs, context, threshold, out_dir, 5);
    model.run();


    // std::string dir = out_dir;
    //
    // em::EVALUATE_DERIVATION eval_deriv = em::EVALUATE_DERIVATION(&model);
    //
    // std::map<std::string, std::vector<int>> em_g, em_i, or_g, or_i, base;
    //
    // std::cout << "Getting Derivs" << std::endl;
    //
    // for (int i = 0; i < graphs.size(); i ++) {
    //     if (i%200 == 0) {
    //         std::cout << i << std::endl;
    //     }
    //
    //     auto graph = graphs[i];
    //     auto code = context->Parse(graph);
    //     if (code == ParserError::kNone) {
    //         ChartItem *root = context->parser->Result();
    //         std::string graph_id = graph.sentence_id;
    //         model.addParentPointerOptimized(root, 0);
    //         model.addRulePointer(root);
    //         std::vector<int> ei = eval_deriv.get_rule_indices(root, em::EVALUATE_DERIVATION::EM_Inside_deriv);
    //         std::vector<int> eg =eval_deriv.get_rule_indices(root, em::EVALUATE_DERIVATION::EM_Greedy_deriv);
    //         std::vector<int> og = eval_deriv.get_rule_indices(root, em::EVALUATE_DERIVATION::Count_Greedy_deriv);
    //         std::vector<int> oi =eval_deriv.get_rule_indices(root, em::EVALUATE_DERIVATION::Count_Inside_deriv);
    //         std::vector<int> b =eval_deriv.get_rule_indices(root, em::EVALUATE_DERIVATION::Baseline_Sample_deriv);
    //         em_g[graph_id] = eg;
    //         em_i[graph_id] = ei;
    //         or_g[graph_id] = og;
    //         or_i[graph_id] = oi;
    //         base[graph_id] = b;
    //         // std::vector<int> g = eval_deriv.Extract
    //
    //         // greedy[graph_id] = g;
    //         // inside[graph_id] = i;
    //     }
    // }
    // std::string eg_f = dir + "eg.txt";
    // std::string ei_f = dir + "ei.txt";
    // std::string og_f = dir + "og.txt";
    // std::string oi_f = dir + "oi.txt";
    // std::string b_f = dir + "b.txt";
    // exportMapToFile(em_g, eg_f);
    // exportMapToFile(em_i, ei_f);
    // exportMapToFile(or_g, og_f);
    // exportMapToFile(or_i, oi_f);
    // exportMapToFile(base, b_f);


    return 0;
}
