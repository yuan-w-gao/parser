////
//// Created by Yuan Gao on 06/12/2024.
////
//
//
//void LoadProbabilities(const std::string& prob_path, AgeMetrics& metrics) {
//    std::ifstream file(prob_path);
//    std::string line;
//    while (std::getline(file, line)) {
//        std::stringstream ss(line);
//        std::string item;
//
//        RuleMetrics rule;
//        std::getline(ss, item, ',');
//        rule.id = std::stoi(item);
//
//        while (std::getline(ss, item, ',')) {
//            rule.probabilities.push_back(std::stod(item));
//        }
//        metrics.rules.push_back(rule);
//    }
//}
//
//bool IsLexicalRule(const RuleMetrics& rule, const SHRG& shrg) {
//    const auto& edges = shrg.fragment.edges;
//    return std::all_of(edges.begin(), edges.end(),
//                       [](const SHRG::Edge& e) { return e.is_terminal; });
//}
//
//double ComputeAvgDerivationDepth(const std::vector<RuleMetrics>& rules,
//                                 const std::vector<SHRG>& shrg_rules) {
//    std::function<int(const SHRG&)> compute_depth = [&](const SHRG& rule) {
//        if (rule.nonterminal_edges.empty()) return 1;
//
//        int max_child_depth = 0;
//        for (const auto* edge : rule.nonterminal_edges) {
//            int child_depth = compute_depth(shrg_rules[edge->label]);
//            max_child_depth = std::max(max_child_depth, child_depth);
//        }
//        return max_child_depth + 1;
//    };
//
//    int total_depth = 0;
//    int count = 0;
//    for (size_t i = 0; i < rules.size(); ++i) {
//        total_depth += compute_depth(shrg_rules[i]);
//        count++;
//    }
//    return count > 0 ? static_cast<double>(total_depth) / count : 0.0;
//}
//
//double ComputeKLDivergence(const std::vector<double>& p, const std::vector<double>& q) {
//    double kl = 0.0;
//    for (size_t i = 0; i < p.size(); ++i) {
//        if (p[i] > 0 && q[i] > 0) {
//            kl += p[i] * (std::log(p[i]) - std::log(q[i]));
//        }
//    }
//    return kl;
//}
//
//void WriteAgeTransitionMetrics(int prev_age, int curr_age, const ContinuityMetrics& metrics) {
//    std::ofstream file("age_transitions.csv", std::ios::app);
//    file << prev_age << ","
//         << curr_age << ","
//         << metrics.shared_rules << ","
//         << metrics.avg_weight_change << ","
//         << metrics.new_rules.size() << ","
//         << metrics.disappeared_rules.size() << "\n";
//}
//
//void WriteCoreGrammarAnalysis(const std::vector<CoreGrammarInfo>& core_rules, size_t num_ages) {
//    std::ofstream file("core_grammar.csv");
//    file << "rule_id,first_appearance,avg_weight,stability,presence_ratio\n";
//
//    for (const auto& rule : core_rules) {
//        double presence_ratio = std::count(rule.presence.begin(),
//                                           rule.presence.end(), true) /
//                                static_cast<double>(num_ages);
//
//        file << rule.rule_id << ","
//             << rule.first_appearance << ","
//             << rule.avg_weight << ","
//             << rule.weight_stability << ","
//             << presence_ratio << "\n";
//    }
//}