#include "em_evaluate.hpp"

namespace shrg::em {
    EM_EVALUATE::EM_EVALUATE(shrg::em::EMBase *em):em(em){
        context = em->getContext();
    }
    std::vector<std::string> EM_EVALUATE::tokenize(const std::string &str) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string word;
        while (ss >> word) {
            tokens.push_back(word);
        }
        return tokens;
    }

    std::map<std::string, int> EM_EVALUATE::getNgrams(const std::vector<std::string> &tokens, int n) {
        std::map<std::string, int> ngrams;
        for (size_t i = 0; i + n <= tokens.size(); ++i) {
            std::string ngram;
            for (int j = 0; j < n; ++j) {
                if (j > 0) ngram += " ";
                ngram += tokens[i + j];
            }
            ngrams[ngram]++;
        }
        return ngrams;
    }

    double EM_EVALUATE::calculateBleuScore(const std::string &candidate, const std::string &reference, int maxN) {
        auto candidateTokens = tokenize(candidate);
        auto referenceTokens = tokenize(reference);

        double precisionSum = 0.0;
        int candidateLength = candidateTokens.size();
        int referenceLength = referenceTokens.size();

        for (int n = 1; n <= maxN; ++n) {
            auto candidateNgrams = getNgrams(candidateTokens, n);
            auto referenceNgrams = getNgrams(referenceTokens, n);

            int matchedNgrams = 0;
            int totalNgrams = candidateNgrams.size();

            for (const auto &pair : candidateNgrams) {
                const std::string &ngram = pair.first;
                int count = pair.second;
                if (referenceNgrams.count(ngram)) {
                    matchedNgrams += std::min(count, referenceNgrams[ngram]);
                }
            }

            double precision = (totalNgrams > 0) ? static_cast<double>(matchedNgrams) / totalNgrams : 0.0;
            precisionSum += std::log(precision + 1e-9);
        }

        double geometricMean = std::exp(precisionSum / maxN);

        double brevityPenalty = (candidateLength > referenceLength) ? 1.0 : std::exp(1.0 - static_cast<double>(referenceLength) / candidateLength);

        return brevityPenalty * geometricMean;
    }

    double EM_EVALUATE::sentence_bleu(EdsGraph &graph, Generator *generator){
        auto code = context->parser->Parse(graph);
        if(code == ParserError::kNone){
            ChartItem *root = context->parser->Result();
            // float d = em->FindBestDerivationWeight(root);
            Derivation derivation;
            std::string sentence;
            generator->Generate(root, derivation, sentence);
//            auto gen = context->Generate();
//            std::string sentence = context->sentence;
            auto reference = graph.lemma_sequence;
            return calculateBleuScore(sentence, reference);
        }

    }

    double EM_EVALUATE::bleu(){
        double bleu = 0.0;
        auto graphs = em->getGraphs();
        for(EdsGraph &graph:graphs){
            bleu += sentence_bleu(graph, context->parser->GetGenerator());
        }
        return bleu/graphs.size();
    }

    double EM_EVALUATE::calculateF1Score(const std::string &candidate, const std::string &reference) {
        auto candidateTokens = tokenize(candidate);
        auto referenceTokens = tokenize(reference);

        std::set<std::string> candidateSet(candidateTokens.begin(), candidateTokens.end());
        std::set<std::string> referenceSet(referenceTokens.begin(), referenceTokens.end());

        int truePositives = 0;
        for (const auto &word : candidateSet) {
            if (referenceSet.find(word) != referenceSet.end()) {
                truePositives++;
            }
        }

        int falsePositives = candidateSet.size() - truePositives;
        int falseNegatives = referenceSet.size() - truePositives;

        double precision = truePositives / static_cast<double>(truePositives + falsePositives);
        double recall = truePositives / static_cast<double>(truePositives + falseNegatives);

        if (precision + recall == 0) {
            return 0.0;
        }

        double f1Score = 2 * (precision * recall) / (precision + recall);
        return f1Score;
    }

    double EM_EVALUATE::f1(){
        double f1 = 0.0;
        auto graphs = em->getGraphs();
        for(EdsGraph &graph:graphs){
            auto code = context->parser->Parse(graph);
            if(code == ParserError::kNone){
                ChartItem *root = context->parser->Result();
                // float d = em->FindBestDerivationWeight(root);
                Derivation derivation;
                std::string sentence;
                context->parser->GetGenerator()->Generate(root, derivation, sentence);
                auto reference = graph.lemma_sequence;
                f1 += calculateF1Score(sentence, reference);
            }
        }
        return f1/graphs.size();
    }

    std::pair<double, double> EM_EVALUATE::f1_and_bleu(){
        double bleu = 0.0;
        double f1 = 0.0;
        auto graphs = em->getGraphs();
        sentences.clear();
        for(int i = 0; i < graphs.size(); i++){
            if(i == 424 || i == 488 || i == 787 || i == 997 || i == 1186 || i == 2309 || i == 2719
                || i == 2843 || i == 2858 || i == 2965 || i == 3092 || i == 3158){
                continue;
            }
            auto graph = graphs[i];
            auto code = context->parser->Parse(graph);
            if(code == ParserError::kNone){
                ChartItem *root = context->parser->Result();
                // float d = em->FindBestDerivationWeight(root);
                Derivation derivation;
                std::string sentence;
                context->parser->GetGenerator()->Generate(root, derivation, sentence);
                sentences.push_back(sentence);
                auto reference = graph.lemma_sequence;
                bleu += calculateBleuScore(sentence, reference);
                f1 += calculateF1Score(sentence, reference);
//                std::cout << i << std::endl;
            }
        }
        return {bleu/graphs.size(), f1/graphs.size()};
    }

    std::vector<std::string> EM_EVALUATE::getSentences(){
        return sentences;
    }


    int EM_EVALUATE::addDerivationNode(Derivation& derivation,
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

    Derivation EM_EVALUATE::extractBestDerivationGreedy(ChartItem *root) {
        Derivation derivation;
        if (!root) return derivation;

        int root_index = addDerivationNode(derivation, root);

        for(auto child_ptr : root->children) {
            ChartItem *best_child = child_ptr;
            double best_weight = child_ptr->rule_ptr ? child_ptr->rule_ptr->log_rule_weight
                                                     : -std::numeric_limits<double>::infinity();
            const SHRG::CFGRule* best_cfg = nullptr;

            ChartItem *curr_ptr = child_ptr->next_ptr;
            while(curr_ptr != child_ptr) {
                double current_weight = curr_ptr->rule_ptr ? curr_ptr->rule_ptr->log_rule_weight
                                                           : -std::numeric_limits<double>::infinity();
                if(current_weight > best_weight) {
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

    Derivation EM_EVALUATE::extractBestDerivationGlobal(ChartItem* root) {
        Derivation derivation;
        if (!root) return derivation;

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

        int root_index = addDerivationNode(derivation, best_item, best_cfg);

        for (auto child : best_item->children) {
            Derivation child_derivation = extractBestDerivationGlobal(child);
            if (!child_derivation.empty()) {
                derivation[root_index].children.push_back(derivation.size());
                derivation.insert(derivation.end(), child_derivation.begin(), child_derivation.end());
            }
        }

        return derivation;
    }

    Derivation EM_EVALUATE::extractBestDerivationByScore(ChartItem* root) {
        Derivation derivation;
        if (!root) return derivation;

        ChartItem* best_item = root;
        double best_score = root->score;
        const SHRG::CFGRule* best_cfg = nullptr;

        ChartItem* current = root->next_ptr;
        while (current != root) {
            if (current->score > best_score) {
                best_score = current->score;
                best_item = current;
                best_cfg = current->rule_ptr ? &current->rule_ptr->cfg_rules[0] : nullptr;
            }
            current = current->next_ptr;
        }

        int root_index = addDerivationNode(derivation, best_item, best_cfg);

        for (auto child : best_item->children) {
            Derivation child_derivation = extractBestDerivationByScore(child);
            if (!child_derivation.empty()) {
                derivation[root_index].children.push_back(derivation.size());
                derivation.insert(derivation.end(), child_derivation.begin(), child_derivation.end());
            }
        }

        return derivation;
    }

    double EM_EVALUATE::compareDerivations(const Derivation& deriv1, const Derivation& deriv2) {
        if (deriv1.empty() || deriv2.empty()) return 0.0;

        double score = 0.0;
        int total_comparisons = 0;

        auto compare_nodes = [](const DerivationNode& n1, const DerivationNode& n2) -> double {
            double node_score = 0.0;
            int node_comparisons = 0;

            if (n1.grammar_ptr && n2.grammar_ptr) {
                if (n1.grammar_ptr->label_hash == n2.grammar_ptr->label_hash) {
                    node_score += 1.0;
                }
                node_comparisons++;
            }

            if (n1.cfg_ptr && n2.cfg_ptr) {
                if (n1.cfg_ptr->label == n2.cfg_ptr->label) {
                    node_score += 1.0;
                }
                node_comparisons++;
            }

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

        for (size_t i = 0; i < std::min(deriv1.size(), deriv2.size()); i++) {
            score += compare_nodes(deriv1[i], deriv2[i]);
            total_comparisons++;

            if (!deriv1[i].children.empty() || !deriv2[i].children.empty()) {
                double structure_score = deriv1[i].children.size() == deriv2[i].children.size() ? 1.0 : 0.0;
                score += structure_score;
                total_comparisons++;
            }
        }

        return total_comparisons > 0 ? score / total_comparisons : 0.0;
    }

    EM_EVALUATE::EvaluationMetrics EM_EVALUATE::evaluateAll_fromSaved_noTree() {
        double bleu = 0.0;
        double f1 = 0.0;
        double greedy_tree_score = 0.0;
        double global_tree_score = 0.0;
        int valid_count = 0;
        sentences.clear();

        auto forests = em->getForests();
        auto lemmas = em->getLemmaSentences();
        for(int i = 0; i < forests.size(); i++) {
            ChartItem *root = forests[i];
            if(!root) continue;

            // em->FindBestDerivationWeight(root);
            Derivation derivation;
            std::string sentence;
            context->parser->GetGenerator()->Generate(root, derivation, sentence);
            sentences.push_back(sentence);

            auto reference = lemmas[i];
            bleu += calculateBleuScore(sentence, reference);
            f1 += calculateF1Score(sentence, reference);
            valid_count++;
        }

        return {
            bleu / valid_count,
            f1 / valid_count,
            0.0,
            0.0,
            sentences
        };
    }


    EM_EVALUATE::EvaluationMetrics EM_EVALUATE::evaluateAll() {
        double bleu = 0.0;
        double f1 = 0.0;
        double greedy_tree_score = 0.0;
        double global_tree_score = 0.0;
        int valid_count = 0;
        sentences.clear();

        auto graphs = em->getGraphs();
        for(int i = 0; i < graphs.size(); i++) {
            auto graph = graphs[i];
            auto code = context->parser->Parse(graph);
            if(code == ParserError::kNone){
                ChartItem *root = context->parser->Result();
                if(!root) continue;

                // Initialize derivation forest
                em->addRulePointer(root);
                em->addChildren(root);

                // Get best derivation for string generation
                // em->FindBestDerivationWeight(root);
                Derivation derivation;
                std::string sentence;
                context->parser->GetGenerator()->Generate(root, derivation, sentence);
                sentences.push_back(sentence);

                // String-based evaluation
                auto reference = graph.lemma_sequence;
                bleu += calculateBleuScore(sentence, reference);
                f1 += calculateF1Score(sentence, reference);

//                // Get all three derivations
//                auto score_deriv = extractBestDerivationByScore(root);
//                auto greedy_deriv = extractBestDerivationGreedy(root);
//                auto global_deriv = extractBestDerivationGlobal(root);

//                // Compare both against score-based derivation
//                greedy_tree_score += compareDerivations(greedy_deriv, score_deriv);
//                global_tree_score += compareDerivations(global_deriv, score_deriv);

                valid_count++;
            }

        }

        return {
            bleu / valid_count,
            f1 / valid_count,
            greedy_tree_score / valid_count,
            global_tree_score / valid_count,
            sentences
        };
    }
}
