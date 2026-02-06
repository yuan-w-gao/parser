//
// Created by Yuan Gao on 11/2/22.
//
#include "manager.hpp"

// #include "python/inside_outside.hpp"
// #include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"
#include "em_framework/em_base.hpp"
#include "em_framework/em.hpp"
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

using namespace shrg;

// void addParentPointerOptimized(ChartItem * root, int level);
// bool has_dup(RuleVector v){
//    std::sort(v.begin(), v.end());
//    auto it = std::unique(v.begin(), v.end());
//    return it == v.end();
//}
//using ItemList = std::vector<ChartItem *>;
//using parent_sib = std::tuple<ChartItem *, std::vector<ChartItem *>>;
//using parents = std::vector<parent_sib>;
//using labelPair = std::pair<LabelHash, LabelHash>;
//
//bool same_list(ItemList &l1, ItemList &l2){
//    if(l1.size() != l2.size()){
//        return false;
//    }
//    set<LabelHash> s1, s2;
//    for(auto i:l1){
//        s1.insert(i->attrs_ptr->grammar_ptr->label_hash);
//    }
//    for(auto i:l2){
//        s2.insert(i->attrs_ptr->grammar_ptr->label_hash);
//    }
//    return s1 == s2;
//}
//
//bool same_parent(parents &p1, parents &p2){
//    if(p1.size() != p2.size()){
//        return false;
//    }
//    set<labelPair> s1, s2;
//    for(auto i:p1){
//        s1.insert({std::get<0>(i)->attrs_ptr->grammar_ptr->label_hash, std::get<1>(i)[0]->attrs_ptr->grammar_ptr->label_hash});
//    }
//    for(auto i:p2){
//        s2.insert({std::get<0>(i)->attrs_ptr->grammar_ptr->label_hash, std::get<1>(i)[0]->attrs_ptr->grammar_ptr->label_hash});
//    }
//    return s1 == s2;
//}
//
//
//bool same_parent(ChartItem *r1, ChartItem *r2){
//    if(r1->rule_visited != r2->rule_visited){
//        return false;
//    }
//    if(r1->rule_visited == shrg::em::EMBase::VISITED){
//        return true;
//    }
//    ChartItem *p1 = r1;
//    ChartItem *p2 = r2;
//    do{
//        if(p1->children.size() != p2->children.size()){
//            return false;
//        }
//        if(p1->parents_sib.size() != p2->parents_sib.size()){
//            return false;
//        }
//        if(!same_list(p1->children, p2->children)){
//            return false;
//        }
//        if(!same_parent(p1->parents_sib, p2->parents_sib)){
//            return false;
//        }
//        p1->rule_visited = shrg::em::EMBase::VISITED;
//        p2->rule_visited = shrg::em::EMBase::VISITED;
//
//        for(int i = 0; i < p1->children.size(); i++){
//            same_parent(p1->children[i], p2->children[i]);
//        }
//
//        p1 = p1->next_ptr;
//        p2 = p2->next_ptr;
//    }while(p1 != r1 || p2 != r2);
//    return true;
//}
//
//
//
//

// std::vector<std::vector<double>> LoadProbabilities(const std::string &prob_file){
//     std::cout << "Loading probs from :" << prob_file << std::endl;
//     std::vector<std::vector<double>> rule_probs;
//     std::ifstream file(prob_file);
//     std::string line;
//     while(std::getline(file, line)){
//         std::stringstream ss(line);
//         std::string item;
//         std::vector<double> probs;
//
//         std::getline(ss, item, ',');
//         while(std::getline(ss, item, ',')){
//             probs.push_back(std::stod(item));
//         }
//
//         rule_probs.push_back(probs);
//     }
//     return rule_probs;
// }
// std::string EdgeSetToString(const EdgeSet& edge_set) {
//     std::string result = "";
//     for (size_t i = 0; i < MAX_GRAPH_EDGE_COUNT; ++i) {
//         result += edge_set[i] ? '1' : '0';
//     }
//     return result;
// }
//
// // Parse a string back to EdgeSet
// EdgeSet StringToEdgeSet(const std::string& str) {
//     EdgeSet edge_set;
//     for (size_t i = 0; i < str.length() && i < MAX_GRAPH_EDGE_COUNT; ++i) {
//         if (str[i] == '1') {
//             edge_set.set(i);
//         }
//     }
//     return edge_set;
// }
// void WriteDerivationInfoMap(const std::map<std::string, DerivationInfo>& deriv_map, const std::string& filename) {
//     std::ofstream out_file(filename);
//     if (!out_file.is_open()) {
//         throw std::runtime_error("Cannot open file for writing: " + filename);
//     }
//
//     for (const auto& [graph_id, info] : deriv_map) {
//         // Write graph ID
//         out_file << "Graph_ID: " << graph_id << "\n";
//
//         // Write rule indices
//         out_file << "Rule_Indices:";
//         for (const auto& idx : info.rule_indices) {
//             out_file << " " << idx;
//         }
//         out_file << "\n";
//
//         // Write edge sets
//         out_file << "Edge_Sets:";
//         for (const auto& edge_set : info.edge_sets) {
//             out_file << " " << EdgeSetToString(edge_set);
//         }
//         out_file << "\n\n";
//     }
//
//     out_file.close();
// }

// std::map<int, std::vector<int>> LoadGoldDerivationIndices(const std::string& filename) {
//     std::map<int, std::vector<int>> result;
//     std::ifstream file(filename);
//
//     if (!file.is_open()) {
//         throw std::runtime_error("Cannot open file: " + filename);
//     }
//
//     std::string line;
//     while (std::getline(file, line)) {
//         // Skip empty lines
//         if (line.empty()) continue;
//
//         std::istringstream line_stream(line);
//         std::string token;
//
//         // Get graph ID
//         std::getline(line_stream, token, ':');
//         int graph_id = std::stoi(token);
//
//         // Get number of indices (we don't actually need this)
//         std::getline(line_stream, token, ':');
//
//         // Get indices
//         std::getline(line_stream, token);
//         std::istringstream indices_stream(token);
//         std::vector<int> indices;
//
//         // Parse comma-separated indices
//         std::string index_str;
//         while (std::getline(indices_stream, index_str, ',')) {
//             // Trim whitespace
//             index_str.erase(0, index_str.find_first_not_of(" \t"));
//             index_str.erase(index_str.find_last_not_of(" \t") + 1);
//
//             if (!index_str.empty()) {
//                 indices.push_back(std::stoi(index_str));
//             }
//         }
//
//         result[graph_id] = indices;
//     }
//
//     file.close();
//     return result;
// }

// double computeInside_score(ChartItem *root){
//     if(root->inside_visited_status == VISITED){
//         return root->log_inside_prob;
//     }
//
//     ChartItem *ptr = root;
//     double log_inside = ChartItem::log_zero;
//
//     do{
//         double curr_log_inside = ptr->score;
//         //        assert(is_negative(curr_log_inside));
//         curr_log_inside = sanitizeLogProb(curr_log_inside);
//
//         double log_children = 0.0;
//         for(ChartItem *child:ptr->children){
//             log_children += computeInside_score(child);
//         }
//         log_children = sanitizeLogProb(log_children);
//
//         //        assert(is_negative(log_children));
//
//         curr_log_inside += log_children;
//         curr_log_inside = sanitizeLogProb(curr_log_inside);
//         //        assert(is_negative(curr_log_inside));
//
//         log_inside = addLogs(log_inside, curr_log_inside);
//         log_inside = sanitizeLogProb(log_inside);
//         //        assert(is_negative(log_inside));
//         //        if(!is_negative(log_inside)){
//         //            std::cout << "?";
//         //        }
//
//         ptr = ptr->next_ptr;
//     }while(ptr != root);
//
//     do{
//         is_negative(log_inside);
//         ptr->log_inside_prob = log_inside;
//         ptr->inside_visited_status = VISITED;
//         ptr = ptr->next_ptr;
//     }while(ptr != root);
//
//     return log_inside;
// }
int count_nodes(ChartItem *root){
    if(root->count_visited_status == ChartItem::kVisited) {
        return 0;
    }
    int count = 0;
    ChartItem *ptr = root;
    do{
        count++;
        ptr = ptr->next_ptr;
        ptr->count_visited_status = ChartItem::kVisited;
        count += count_nodes(ptr);
    }while (ptr != root);
}

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
    // auto SHRG::Generator* generator = context->parser->GetGenerator();
    std::vector<ChartItem*> forest;
    for (auto graph:graphs) {
        auto code = context->parser->Parse(graph);
        if(code == ParserError::kNone) {
            ChartItem *root = context->parser->Result();

        }
    }


    return 0;
}

