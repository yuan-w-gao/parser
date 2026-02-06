// em_evaluate.hpp
#pragma once

#include <unordered_map>
#include <vector>
#include <limits>
#include <cmath>
#include <string>
#include <map>
#include <fstream>
#include <sstream>

#include "../em_base.hpp"

namespace shrg::em {

class EM_EVALUATE {
  public:
    explicit EM_EVALUATE(EMBase *em);

    struct EvaluationMetrics {
        double bleu;
        double f1;
        double greedy_tree_score;
        double global_tree_score;
        std::vector<std::string> generated_sentences;
    };

    // String evaluation methods
    double sentence_bleu(EdsGraph &graph, Generator *generator);
    double bleu();
    double f1();
    std::pair<double, double> f1_and_bleu();
    std::vector<std::string> getSentences();

    // Tree evaluation methods
    Derivation extractBestDerivationGreedy(ChartItem* root);
    Derivation extractBestDerivationGlobal(ChartItem* root);
    double compareDerivations(const Derivation& deriv1, const Derivation& deriv2);
    Derivation extractBestDerivationByScore(ChartItem* root);

    EvaluationMetrics evaluateAll();
    EM_EVALUATE::EvaluationMetrics evaluateAll_fromSaved_noTree();

  private:
    EMBase* em;
    Context* context;
    std::vector<std::string> sentences;

    // Helper functions
    std::vector<std::string> tokenize(const std::string &str);
    std::map<std::string, int> getNgrams(const std::vector<std::string> &tokens, int n);
    double calculateBleuScore(const std::string &candidate, const std::string &reference, int maxN = 4);
    double calculateF1Score(const std::string &candidate, const std::string &reference);
    int addDerivationNode(Derivation& derivation, 
                          ChartItem* item,
                          const SHRG::CFGRule* best_cfg = nullptr);
};

} // namespace shrg::em