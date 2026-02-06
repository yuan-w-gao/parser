//
// Created by Yuan on 9/26/25.
//
//
// Created by Yuan Gao on 11/2/22.
//
#include "manager.hpp"

// #include "python/inside_outside.hpp"
// #include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"
#include "em_framework/em_base.hpp"
#include "em_framework/em.hpp"
#include "em_framework/find_derivations.hpp"

// // #include "em_framework/em_hmm.hpp"
// #include "python/find_best_derivation.cpp"
// #include "test.cpp"
// #include "em_framework/em_evaluate/eval_deriv.hpp"
// #include "em_framework/em_utils.hpp"
// #include "em_framework/variational_inference.hpp"

#include <set>
#include <iostream>
#include <map>
#include <chrono>
#include <random>
#include <string>
#include <limits>
#include <cctype>
#include <cerrno>
#include <cmath>

using namespace shrg;

// Helper for robust double parsing (handles subnormals, inf, nan)
static inline void trim_inplace(std::string& s) {
    auto issp = [](unsigned char c){ return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back()))  s.pop_back();
}

static inline bool parse_double_robust(const std::string& raw, double& out) {
    std::string s = raw;
    trim_inplace(s);
    if (s.empty()) return false;

    if (s == "inf" || s == "+inf") { out = std::numeric_limits<double>::infinity(); return true; }
    if (s == "-inf") { out = -std::numeric_limits<double>::infinity(); return true; }
    if (s == "nan") { out = std::numeric_limits<double>::quiet_NaN(); return true; }

    errno = 0;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end != s.c_str();
}

std::vector<std::vector<double>> LoadProbabilities(const std::string &prob_file){
    std::vector<std::vector<double>> rule_probs;
    std::ifstream file(prob_file);
    std::string line;
    while(std::getline(file, line)){
        std::stringstream ss(line);
        std::string item;
        std::vector<double> probs;

        std::getline(ss, item, ',');
        while(std::getline(ss, item, ',')){
            double val;
            if (parse_double_robust(item, val)) {
                probs.push_back(val);
            }
        }

        rule_probs.push_back(probs);
    }
    return rule_probs;
}
std::string EdgeSetToString(const shrg::EdgeSet& edge_set) {
    std::string result = "";
    for (size_t i = 0; i < shrg::MAX_GRAPH_EDGE_COUNT; ++i) {
        result += edge_set[i] ? '1' : '0';
    }
    return result;
}

// Parse a string back to EdgeSet
shrg::EdgeSet StringToEdgeSet(const std::string& str) {
    shrg::EdgeSet edge_set;
    for (size_t i = 0; i < str.length() && i < shrg::MAX_GRAPH_EDGE_COUNT; ++i) {
        if (str[i] == '1') {
            edge_set.set(i);
        }
    }
    return edge_set;
}
void WriteDerivationInfoMap(const std::map<std::string, shrg::DerivationInfo>& deriv_map, const std::string& filename) {
    std::ofstream out_file(filename);
    if (!out_file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }

    for (const auto& [graph_id, info] : deriv_map) {
        // Write graph ID
        out_file << "Graph_ID: " << graph_id << "\n";

        // Write rule indices
        out_file << "Rule_Indices:";
        for (const auto& idx : info.rule_indices) {
            out_file << " " << idx;
        }
        out_file << "\n";

        // Write edge sets
        out_file << "Edge_Sets:";
        for (const auto& edge_set : info.edge_sets) {
            out_file << " " << EdgeSetToString(edge_set);
        }
        out_file << "\n\n";
    }

    out_file.close();
}

std::map<int, std::vector<int>> LoadGoldDerivationIndices(const std::string& filename) {
    std::map<int, std::vector<int>> result;
    std::ifstream file(filename);

    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // std::cout << "Processing line: '" << line << "'" << std::endl;

        std::istringstream line_stream(line);
        std::string token;

        // Get graph ID
        std::getline(line_stream, token, ':');
        int graph_id = std::stoi(token);
        if (graph_id == 118) {
            std::cout << graph_id << std::endl;
        }
        // std::cout << "Graph ID: " << graph_id << std::endl;

        // Get number of indices (we don't actually need this)
        std::getline(line_stream, token, ':');
        // std::cout << "Number field: '" << token << "'" << std::endl;

        // Get indices
        std::getline(line_stream, token);
        // std::cout << "Indices string: '" << token << "'" << std::endl;

        std::istringstream indices_stream(token);
        std::vector<int> indices;

        // Parse comma-separated indices
        std::string index_str;
        while (std::getline(indices_stream, index_str, ',')) {
            // std::cout << "Raw index_str: '" << index_str << "'" << std::endl;

            // Trim whitespace
            index_str.erase(0, index_str.find_first_not_of(" \t"));
            index_str.erase(index_str.find_last_not_of(" \t") + 1);

            // std::cout << "Trimmed index_str: '" << index_str << "'" << std::endl;

            if (!index_str.empty()) {
                int index = std::stoi(index_str);
                // std::cout << "Parsed index: " << index << std::endl;
                indices.push_back(index);
                // std::cout << "Vector size after push_back: " << indices.size() << std::endl;
            }
        }

        // std::cout << "Final indices vector size: " << indices.size() << std::endl;
        // std::cout << "Indices contents: ";
        // for (int idx : indices) {
            // std::cout << idx << " ";
        // }
        // std::cout << std::endl;

        result[graph_id] = indices;
        // std::cout << "Added to result map for graph_id " << graph_id << std::endl;
        // std::cout << "Result map now has " << result.size() << " entries" << std::endl;
        // std::cout << "---" << std::endl;
    }

    file.close();
    return result;
}




int main(int argc, char *argv[]) {
    clock_t t1,t2, t3, t4;

    auto *manager = &shrg::Manager::manager;
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
    std::vector<shrg::SHRG *>shrg_rules = manager->shrg_rules;
    // auto Generator* generator = context->parser->GetGenerator();
    std::vector<shrg::ChartItem*> forest;
    // for (auto graph:graphs) {
    //     auto code = context->parser->Parse(graph);
    //     if(code == ParserError::kNone) {
    //         ChartItem *root = context->parser->Result();
    //         forest.push_back(root);
    //         addParentPointer(root, generator, 0);
    //         addRulePointer(root, shrg_rules);
    //         double pw = computeInside(root);
    //         computeOutside(root);
    //     }
    // }
    // EM(shrg_rules, graphs, context, 0.05);

    // std::cout << "Rules Len: " << shrg_rules.size() << std::endl;
    std::string outDir = std::string(argv[4]);
    std::string gold_file = argv[6];
    // int num_iteration = 1;
    shrg::em::EM model = shrg::em::EM(shrg_rules, graphs, context, 10, argv[4], 5);
    // // shrg::vi::VariationalInference model = shrg::vi::VariationalInference(shrg_rules, graphs, context, 30);
    // model.run();
    std::vector<std::vector<double>> probs = LoadProbabilities(argv[5]);

    bool equals = probs.size() == shrg_rules.size();

    std::cout << argv[1] << "prob_size: " << probs.size() << "; rule_size: " << shrg_rules.size() << std::endl;
    //
    int num_iteration = probs[0].size() - 1;

    for(size_t i = 0; i < shrg_rules.size(); i++){
        auto rule = shrg_rules[i];
        rule->log_rule_weight = probs[i][num_iteration];
    }
    //
    //
    // // em::EVALUATE_DERIVATION eval_deriv = em::EVALUATE_DERIVATION(&model);
    //
    // // std::map<std::string, std::vector<int>> em_g, em_i, or_g, or_i, base;
    //
    // std::cout << "Getting Derivs" << std::endl;
    //
    std::map<std::string, shrg::DerivationInfo> em_map, oracle_map, gold_map;
    std::map<std::string, shrg::DerivationInfo> base_map;
    //
    std::map<int, std::vector<int>> gold_deriv = LoadGoldDerivationIndices(gold_file);
    //
    //
    std::random_device rd;
    std::mt19937 rng(rd());
    for( int i = 0; i < graphs.size(); i++){
        if (i%200 == 0) {
            std::cout << i << std::endl;
        }
        auto graph = graphs[i];
        auto code = context->parser->Parse(graph);
        if(code == shrg::ParserError::kNone) {
            shrg::ChartItem *root = context->parser->Result();
            model.addParentPointerOptimized(root, 0);
            model.addRulePointer(root);
            // model.computeInside(root);
            shrg::DerivationInfo oracle= ExtractRuleIndicesAndEdges_ScoreGreedy(root);
            shrg::DerivationInfo em = ExtractRuleIndicesAndEdges_EMGreedy(root);
            shrg::DerivationInfo base = ExtractDerivation_uniform(root);
            int y = std::stoi(graph.sentence_id.substr(graph.sentence_id.find('-') + 1));
            std::vector<int> gold_ind = gold_deriv[y];
            // if (y == 1) {
            //     std::cout << "here";
            // }
            std::optional<shrg::DerivationInfo> gold = ExtractGoldDerivationTree(root, gold_ind);
            if (gold){
                gold_map[graph.sentence_id] = *gold;
            }else{
                std::cout << "Could not find valid derivation for graph " << graph.sentence_id << std::endl;
            }
            base_map[graph.sentence_id] = base;
            oracle_map[graph.sentence_id] = oracle;
            em_map[graph.sentence_id] = em;
        }
    }
    WriteDerivationInfoMap(base_map, outDir+"base_edges.txt");
    WriteDerivationInfoMap(oracle_map, outDir+"oracle_edges.txt");
    WriteDerivationInfoMap(em_map, outDir+"em_edges.txt");
    WriteDerivationInfoMap(gold_map, outDir+"gold_edges.txt");



    return 0;
}
