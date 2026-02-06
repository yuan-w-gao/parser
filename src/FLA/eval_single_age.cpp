//
// Created by Yuan Gao on 09/12/2024.
//
#include <fstream>
#include <limits>
#include <cctype>
#include <cerrno>
#include <cmath>

#include "eval_single_age.hpp"

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

void SingleAgeEvaluator::LoadProbabilities(std::string &prob_file){
    std::vector<std::vector<double>> rule_probs;
    std::ifstream file(prob_file);
    std::string line;
    bool rule_metrics_init;
    if(rule_metrics.size() > 0){
        rule_metrics_init = true;
    }else{
        rule_metrics_init = false;
    }
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
        if(rule_metrics_init){
            rule_metrics[rule_probs.size() -1].probabilities = probs;
        }
    }
    rule_probs = rule_probs;
}

void SingleAgeEvaluator::TrackSignificantWeightChanges(){
    for(size_t i = 0; i < rule_probs.size(); i++){

    }
}


void SingleAgeEvaluator::LoadRules(std::vector<SHRG *> &shrg_rules){
    for(const auto &rule:shrg_rules){
        RuleMetrics m;
        if(isLexicalRule(rule)) {
            m.is_lexical = true;
            metrics.lexical_rules++;
        }else{
            m.is_lexical = false;
            metrics.structural_rules++;
        }
    }
}

SingleAgeEvaluator::Metrics SingleAgeEvaluator::Evaluate(const std::vector<SHRG> &rules, const std::string &probability_file, int month) {
    Metrics metrics;
    metrics.month = month;



}