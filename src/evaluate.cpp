//
// Created by Yuan Gao on 09/12/2024.
//
//#include "em_framework/metrics.hpp"
#include "manager.hpp"
//#include <__filesystem/directory_iterator.h>

#include <fstream>
#include "manager.hpp"
#include <random>
#include <memory>
#include <limits>
#include <cctype>
#include <cerrno>
#include <cmath>
#include "em_framework/find_derivations.hpp"
#include "em_framework/em.hpp"
#include "em_framework/em_batch.hpp"
#include "em_framework/em_online.hpp"
#include "em_framework/em_viterbi.hpp"

using namespace shrg;

// Helper functions for robust double parsing (handles subnormals, inf, nan)
static inline void trim_inplace(std::string& s) {
    auto issp = [](unsigned char c){ return std::isspace(c); };
    while (!s.empty() && issp(s.front())) s.erase(s.begin());
    while (!s.empty() && issp(s.back()))  s.pop_back();
    if (!s.empty() && s.back() == '\r') s.pop_back();
}

static inline std::string lower_copy(std::string s) {
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

static inline bool parse_double_robust(const std::string& raw, double& out) {
    std::string s = raw;
    trim_inplace(s);
    if (s.empty()) return false;

    std::string lo = lower_copy(s);
    if (lo == "inf" || lo == "+inf" || lo == "infinity" || lo == "+infinity") {
        out = std::numeric_limits<double>::infinity();
        return true;
    }
    if (lo == "-inf" || lo == "-infinity") {
        out = -std::numeric_limits<double>::infinity();
        return true;
    }
    if (lo == "nan" || lo == "+nan" || lo == "-nan") {
        out = std::numeric_limits<double>::quiet_NaN();
        return true;
    }

    // Use strtod instead of stod to handle subnormals without throwing
    errno = 0;
    char* end = nullptr;
    const double v = std::strtod(s.c_str(), &end);
    if (end == s.c_str()) return false;  // No conversion

    // For underflow (subnormals), strtod returns the value and sets errno=ERANGE
    // We accept the value rather than failing
    out = v;
    return true;
}

std::vector<std::vector<double>> LoadProbabilities(const std::string& prob_file) {
    std::cout << "Loading probs from :" << prob_file << std::endl;
    std::ifstream file(prob_file);
    if (!file) throw std::runtime_error("Cannot open: " + prob_file);

    std::vector<std::vector<double>> rule_probs;
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string item;
        std::vector<double> probs;

        std::getline(ss, item, ',');  // Skip first field (rule id)

        while (std::getline(ss, item, ',')) {
            double val;
            if (parse_double_robust(item, val)) {
                probs.push_back(val);
            }
        }

        rule_probs.push_back(std::move(probs));
    }

    return rule_probs;
}

float FindBestDerivation(Generator *generator, ChartItem *root_ptr) {
    if (!root_ptr) return -std::numeric_limits<float>::infinity();
    if (root_ptr->status == VISITED)
        return root_ptr->score;

    float log_sum_scores = 0;
    ChartItem *ptr = root_ptr;
    do {
        log_sum_scores += std::exp(ptr->score);
        ptr = ptr->next_ptr;
    } while (ptr && ptr != root_ptr);
    log_sum_scores = std::log(log_sum_scores);

    float max_score = -std::numeric_limits<float>::infinity();
    ChartItem *max_subgraph_ptr = root_ptr;

    ptr = root_ptr;
    do {
        float current_score = ptr->score - log_sum_scores;

        if (ptr->attrs_ptr && ptr->attrs_ptr->grammar_ptr) {
            const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            for (auto edge_ptr : grammar_ptr->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                if (child) {
                    current_score += FindBestDerivation(generator, child);
                }
            }
        }

        if (max_score < current_score) {
            max_score = current_score;
            max_subgraph_ptr = ptr;
        }

        ptr = ptr->next_ptr;
    } while (ptr != root_ptr);

    if (max_subgraph_ptr != root_ptr)
        root_ptr->Swap(*max_subgraph_ptr);

    root_ptr->status = VISITED;
    root_ptr->score = max_score;
    return root_ptr->score;
}

float FindBestDerivationWeight(ChartItem *root_ptr, Generator *generator) {
    if (!root_ptr) return -std::numeric_limits<float>::infinity();
    if (root_ptr->status == VISITED)
        return root_ptr->score;

    ChartItem *ptr = root_ptr;

    double max_weight = -std::numeric_limits<double>::infinity();
    ChartItem *max_subgraph_ptr = root_ptr;

    ptr = root_ptr;
    do {
        double current_weight = -std::numeric_limits<double>::infinity();
        if (ptr->attrs_ptr && ptr->attrs_ptr->grammar_ptr) {
            current_weight = ptr->attrs_ptr->grammar_ptr->log_rule_weight;

            const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
            for (auto edge_ptr : grammar_ptr->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                if (child) {
                    current_weight += FindBestDerivationWeight(child, generator);
                }
            }
        }

        if (max_weight < current_weight) {
            max_weight = current_weight;
            max_subgraph_ptr = ptr;
        }

        ptr = ptr->next_ptr;
    } while (ptr && ptr != root_ptr);

    if (max_subgraph_ptr != root_ptr)
        root_ptr->Swap(*max_subgraph_ptr);

    root_ptr->status = VISITED;
    root_ptr->score = max_weight;
    return root_ptr->score;
}
//
// void SampleDerivationRule(ChartItem *root_ptr, Generator *generator, std::mt19937& rng) {
//     if (root_ptr->status == VISITED)
//         return ;
//
//
//     // Count number of rules
//     int num_rules = 0;
//     ChartItem *ptr = root_ptr;
//     do {
//         num_rules++;
//         ptr = ptr->next_ptr;
//     } while (ptr != root_ptr);
//
//     // Random index
//     std::uniform_int_distribution<int> dist(0, num_rules - 1);
//     int random_idx = dist(rng);
//
//     // Get rule at random index
//     ptr = root_ptr;
//     for(int i = 0; i < random_idx; i++)
//         ptr = ptr->next_ptr;
//
//
//     const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
//     for (auto edge_ptr : grammar_ptr->nonterminal_edges) {
//         SampleDerivationRule(generator->FindChartItemByEdge(ptr, edge_ptr), generator, rng);
//     }
//
//     if (ptr != root_ptr) {
//         root_ptr->Swap(*ptr);
//     }
//
//     return ;
// }

#include <fstream>
#include <string>
#include <map>
#include <sstream>

// Write a single EdgeSet to string in a readable format
std::string EdgeSetToString(const EdgeSet& edge_set) {
    std::string result = "";
    for (size_t i = 0; i < MAX_GRAPH_EDGE_COUNT; ++i) {
        result += edge_set[i] ? '1' : '0';
    }
    return result;
}

// Parse a string back to EdgeSet
EdgeSet StringToEdgeSet(const std::string& str) {
    EdgeSet edge_set;
    for (size_t i = 0; i < str.length() && i < MAX_GRAPH_EDGE_COUNT; ++i) {
        if (str[i] == '1') {
            edge_set.set(i);
        }
    }
    return edge_set;
}

// Write the map to a file
void WriteDerivationInfoMap(const std::map<std::string, DerivationInfo>& deriv_map, const std::string& filename) {
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

// Read the map from a file
std::map<int, DerivationInfo> ReadDerivationInfoMap(const std::string& filename) {
    std::map<int, DerivationInfo> result;
    std::ifstream in_file(filename);
    if (!in_file.is_open()) {
        throw std::runtime_error("Cannot open file for reading: " + filename);
    }

    std::string line;
    int current_graph_id = -1;
    DerivationInfo current_info;

    while (std::getline(in_file, line)) {
        if (line.empty()) {
            if (current_graph_id != -1) {
                result[current_graph_id] = current_info;
                current_graph_id = -1;
                current_info = DerivationInfo();
            }
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "Graph_ID:") {
            iss >> current_graph_id;
        }
        else if (prefix == "Rule_Indices:") {
            int index;
            while (iss >> index) {
                current_info.rule_indices.push_back(index);
            }
        }
        else if (prefix == "Edge_Sets:") {
            std::string edge_set_str;
            while (iss >> edge_set_str) {
                current_info.edge_sets.push_back(StringToEdgeSet(edge_set_str));
            }
        }
    }

    // Handle the last entry if exists
    if (current_graph_id != -1) {
        result[current_graph_id] = current_info;
    }

    in_file.close();
    return result;
}


void writeStringsToFile(const std::vector<std::string>& strings, const std::string& filename) {
    std::ofstream outFile(filename); // Open the file for writing

    if (!outFile) { // Check if the file was opened successfully
        std::cerr << "Error: Could not open the file " << filename << " for writing." << std::endl;
        return;
    }

    for (const auto& str : strings) { // Iterate over the vector of strings
        outFile << str << std::endl; // Write each string followed by a newline
    }

    outFile.close(); // Close the file
}
ChartItem* SampleDerivationTree(ChartItem* root_ptr, std::mt19937& gen) {
    if (!root_ptr) return nullptr;
    if (root_ptr->em_greedy_score == VISITED)
        return root_ptr;

    ChartItem* ptr = root_ptr;
    std::vector<ChartItem*> choices;

    // Collect all possible choices
    do {
        choices.push_back(ptr);
        ptr = ptr->next_ptr;
    } while (ptr && ptr != root_ptr);

    if (choices.empty()) return root_ptr;

    // Uniformly sample one choice
    std::uniform_int_distribution<> dist(0, choices.size() - 1);
    int chosen_idx = dist(gen);
    ChartItem* chosen_ptr = choices[chosen_idx];

    // If we chose a different rule than the root, swap it to the root
    if (chosen_ptr != root_ptr) {
        root_ptr->Swap(*chosen_ptr);
    }

    // Recursively sample children
    for (auto child : root_ptr->children) {
        if (child) {
            SampleDerivationTree(child, gen);
        }
    }

    root_ptr->em_greedy_score = VISITED;
    return root_ptr;
}

int main(int argc, char* argv[]) {
    auto *manager = &Manager::manager;
    manager->Allocate(1);
    if (argc < 7){
        std::cout << "Usage: " << argv[0] << " <config> <grammars> <graphs> <output_dir> <probabilities> <model_type>" << std::endl;
        std::cout << "model_type: em, batch, online, viterbi" << std::endl;
        return 1;
    }

    manager->LoadGrammars(argv[2]);
    manager->LoadGraphs(argv[3]);
    auto &context = manager->contexts[0];
    context->Init(argv[1], false, 100);
    std::string outDir = std::string(argv[4]);

    std::vector<std::vector<double>> probs = LoadProbabilities(argv[5]);
    std::vector<SHRG *> shrg_rules = manager->shrg_rules;
    std::string model_type = argv[6];

    bool equals = probs.size() == shrg_rules.size();

    std::cout << argv[1] << "prob_size: " << probs.size() << "; rule_size: " << shrg_rules.size() << std::endl;

    if (probs.empty() || probs[0].empty()) {
        std::cerr << "Error: probs is empty or has no iterations" << std::endl;
        return 1;
    }
    int num_iteration = probs[0].size() - 1;
    std::cout << "num_iteration: " << num_iteration << std::endl;

    int null_count = 0;
    for(size_t i = 0; i < shrg_rules.size(); i++){
        auto rule = shrg_rules[i];
        if (rule != nullptr) {
            rule->log_rule_weight = probs[i][num_iteration];
        } else {
            null_count++;
        }
    }
    std::cout << "Assigned weights. null_count: " << null_count << std::endl;
    std::vector<std::string> sentences;
    std::vector<std::string> baselines;
    // std::vector<std::string> first_iter;
    Generator *generator = context->parser->GetGenerator();
    std::vector<std::string> lemmas;
    std::vector<std::string> oracles;
    std::vector<std::string> originals;
    std::map<std::string, DerivationInfo> em_map, oracle_map;

    std::random_device rd;
    std::mt19937 rng(rd());
    //    for( auto &graph:manager->edsgraphs){
    //     auto code = context->parser->Parse(graph);
    //     if(code == ParserError::kNone) {
    //         ChartItem *root = context->parser->Result();
    //         DerivationInfo em = ExtractRuleIndicesAndEdges_EMGreedy(root);
    //         DerivationInfo count = ExtractRuleIndicesAndEdges_CountGreedy(root);
    //         em_map[graph.sentence_id] = em;
    //         oracle_map[graph.sentence_id] = count;
    //     }
    // }
    //    WriteDerivationInfoMap(em_map,outDir+"em_edges.txt" );
    //    WriteDerivationInfoMap(oracle_map, outDir+"oracle_edges.txt");
    //    }
    

    // Create the appropriate model based on argument
    shrg::em::EMBase* model = nullptr;
    if (model_type == "em") {
        outDir += "em/";
        model = new shrg::em::EM(shrg_rules, manager->edsgraphs, context, 30, outDir, 5);
    } else if (model_type == "batch") {
        outDir += "batch_em/";
        model = new shrg::em::BatchEM(shrg_rules, manager->edsgraphs, context, 30, 10);
    } else if (model_type == "online") {
        outDir += "online_em/";
        model = new shrg::em::OnlineEM(shrg_rules, manager->edsgraphs, context, 30);
    } else if (model_type == "viterbi") {
        outDir += "viterbi_em/";
        model = new shrg::em::ViterbiEM(shrg_rules, manager->edsgraphs, context, 30);
    } else {
        std::cout << "Unknown model type: " << model_type << std::endl;
        std::cout << "Valid types: em, batch, online, viterbi" << std::endl;
        return 1;
    }
    // Combined loop: parse each graph twice instead of three times
    // - First parse: baseline (uses em_greedy_score) + EM (uses status) - different markers, no conflict
    // - Second parse: oracle (uses status) - needs fresh forest since EM already set status
    std::cout << "Processing " << manager->edsgraphs.size() << " graphs" << std::endl;
    for (auto &graph : manager->edsgraphs) {
        // First parse: baseline + EM weight-based generation
        auto code = context->parser->Parse(graph);

        if (code == ParserError::kNone) {
            ChartItem *root = context->parser->Result();
            if (root) {
                model->addParentPointerOptimized(root, 0);
                model->addRulePointer(root);

                // Baseline: uniform sampling (uses em_greedy_score marker)
                SampleDerivationTree(root, rng);

                Derivation deriv_base;
                std::string base;
                generator->Generate(root, deriv_base, base);
                baselines.push_back(base);

                // EM: weight-based (uses status marker - no conflict with em_greedy_score)
                FindBestDerivationWeight(root, generator);

                Derivation deriv_em;
                std::string em;
                generator->Generate(root, deriv_em, em);
                sentences.push_back(em);

                lemmas.push_back(graph.lemma_sequence);
                originals.push_back(graph.sentence);
            }
        }

        // Second parse: oracle probability-based generation (needs fresh status markers)
        code = context->parser->Parse(graph);

        if (code == ParserError::kNone) {
            ChartItem *root = context->parser->Result();
            if (root) {
                model->addParentPointerOptimized(root, 0);
                model->addRulePointer(root);

                FindBestDerivation(generator, root);

                Derivation deriv_or;
                std::string oracle;
                generator->Generate(root, deriv_or, oracle);
                oracles.push_back(oracle);
            }
        }
    }

 // for( auto &graph:manager->edsgraphs){
 //     auto code = context->parser->Parse(graph);

 //         ChartItem *root = context->parser->Result();
 //         FindBestDerivationWeight(root, generator);
 //         Derivation deriv;
 //         std::string base;
 //         generator->Generate(root, deriv, base);
 //         sentences.push_back(base);
 //     }
 // }


    // std::cout << "num equals: " << num_equals << std::endl;
    std::cout << "num sentences generated: " << baselines.size() << std::endl;
    writeStringsToFile(oracles, outDir+"oracle.txt");
    writeStringsToFile(lemmas, outDir+"reference_sentences.txt");
    writeStringsToFile(sentences, outDir+"em.txt");
    writeStringsToFile(baselines, outDir+"baselines.txt");
    writeStringsToFile(originals, outDir+"original_sentences.txt");

    return 0;
}
