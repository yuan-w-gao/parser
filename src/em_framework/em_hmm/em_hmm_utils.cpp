//
// Created by Yuan Gao on 24/06/2024.
//
#pragma once

#include <random>
#include "em_hmm_utils.hpp"

namespace shrg::em{

//------------------------------LOGGING Functions-----------------------------
    void log_matrix_to_file(const std::vector<std::vector<double>>& m, std::ofstream& log_stream) {
        for (size_t i = 0; i < m.size(); i++) {
            for (size_t j = 0; j < m[i].size(); j++) {
                log_stream << m[i][j] << " ";
            }
            log_stream << "\n";
        }
    }

    void log_vector_to_file(const std::vector<double>& vec, std::ofstream& log_stream) {
        for (const auto& elem : vec) {
            log_stream << elem << " ";
        }
        log_stream << "\n";
    }

    void print_vector(std::vector<double> p){
        for(size_t i = 0; i < p.size(); i++){
            std::cout << p[i] << " ";
            std::cout << "\n";
        }
    }

    void print_matrix(std::vector<std::vector<double>> m){
        for(size_t i = 0; i < m.size(); i++){
            for(size_t j = 0; j < m[i].size(); j++){
                std::cout << m[i][j] << " ";
            }
            std::cout << "\n";
        }
    }

//-----------------------------PROBABILITY OPERATION--------------------------------

    double add_logs(double log_a, double log_b) {
        if (log_a == -std::numeric_limits<double>::infinity()) return log_b;
        if (log_b == -std::numeric_limits<double>::infinity()) return log_a;
        return log_a > log_b ? log_a + log(1.0 + exp(log_b - log_a)) : log_b + log(1.0 + exp(log_a - log_b));
    }

    void normalize_weights(RuleVector& rules) {
        double log_sum = -std::numeric_limits<double>::infinity();
        for (const auto& rule : rules) {
            log_sum = add_logs(log_sum, rule->log_rule_weight);
        }
        for (auto& rule : rules) {
            rule->log_rule_weight -= log_sum;
        }
    }

    std::vector<double> normalize_log_probs(const std::vector<double>& log_probs) {
        double log_sum = -std::numeric_limits<double>::infinity();
        for (double log_prob : log_probs) {
            log_sum = add_logs(log_sum, log_prob);
        }
        std::vector<double> normalized_log_probs(log_probs.size());
        for (size_t i = 0; i < log_probs.size(); ++i) {
            normalized_log_probs[i] = log_probs[i] - log_sum;
        }
        return normalized_log_probs;
    }

    std::vector<double> normalize_probs(const std::vector<double>& probs){
        double sum = 0.0;
        for(double p:probs){
            sum += p;
        }

        std::vector<double> normalized(probs.size());
        for (size_t i = 0; i < probs.size(); i++){
            normalized[i] = probs[i] / sum;
        }
        return normalized;
    }

//-----------------------------MATRIX CALCULATION------------------------------------

    double frobenius_norm(const std::vector<std::vector<double>>& matrix) {
        double norm = 0.0;
        for (const auto & i : matrix) {
            for (size_t j = 0; j < i.size(); ++j) {
                norm += i[j] * i[j];
            }
        }
        return std::sqrt(norm);
    }

    double generate_gaussian_noise(double mean, double stddev) {
        static std::random_device rd;
        static std::mt19937 generator(rd());
        std::normal_distribution<double> distribution(mean, stddev);
        return distribution(generator);
    }
    double cross_entropy_log_q(const std::vector<double>& p, const std::vector<double>& log_q) {
        double entropy = 0.0;

        std::vector<double> normalized_log_q = normalize_log_probs(log_q);

        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i] > 0) {
                double log_qi = normalized_log_q[i];
                entropy -= p[i] * log_qi;
            }
        }
        return entropy;
    }

    //-----------------------------LOADING FROM FILES------------------------------------
    void load_HMM_parameters(const std::string& filename, int num_states, int num_symbols,
                                   std::vector<std::vector<double>>& A,
                                   std::vector<std::vector<double>>& B) {
        std::ifstream file(filename);
        std::string line;
        bool reading_transition = false;
        bool reading_emission = false;
        int current_state = 0;

        if (!file.is_open()) {
            std::cerr << "Failed to open the file." << std::endl;
            return;
        }

        while (std::getline(file, line)) {
            if (line.find("Transition probabilities:") != std::string::npos) {
                reading_transition = true;
                reading_emission = false;
                current_state = 0;
                continue;
            }
            if (line.find("Emission probabilities:") != std::string::npos) {
                reading_transition = false;
                reading_emission = true;
                current_state = 0;
                continue;
            }
            if (reading_transition && current_state < num_states) {
                std::istringstream iss(line);
                double value;
                int symbol_index = 0;
                while (iss >> value) {
                    A[current_state][symbol_index] = value;
                    symbol_index++;
                }
                current_state++;
            }
            if (reading_emission && current_state < num_states) {
                std::istringstream iss(line);
                double value;
                int symbol_index = 0;
                while (iss >> value) {
                    B[current_state][symbol_index] = value;
                    symbol_index++;
                }
                current_state++;
            }
        }

        file.close();
    }

    void load_true_prob(const std::string& filename, std::vector<std::vector<double>> &transition_true,
                      std::vector<std::vector<double>> &emission_true,
                      std::vector<double> &end_prob_true) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Could not open the file!" << std::endl;
            return;
        }

        std::string line;
        enum Section {
            NONE,
            TRANSITION_MATRIX,
            END_PROBABILITIES,
            OMISSION_MATRIX
        } currentSection = NONE;

        while (getline(file, line)) {
            if (line.empty()) {
                continue; // Skip empty lines
            }

            if (line == "Transition Matrix:") {
                currentSection = TRANSITION_MATRIX;
                continue;
            } else if (line == "End Probabilities:") {
                currentSection = END_PROBABILITIES;
                continue;
            } else if (line == "Omission Matrix:") {
                currentSection = OMISSION_MATRIX;
                continue;
            }

            std::stringstream ss(line);
            if (currentSection == TRANSITION_MATRIX) {
                std::vector<double> row;
                double value;
                while (ss >> value) {
                    row.push_back(value);
                }
                transition_true.push_back(row);
            } else if (currentSection == END_PROBABILITIES) {
                double value;
                while (ss >> value) {
                    end_prob_true.push_back(value);
                }
            } else if (currentSection == OMISSION_MATRIX) {
                std::vector<double> row;
                double value;
                while (ss >> value) {
                    row.push_back(value);
                }
                emission_true.push_back(row);
            }
        }

        file.close();
    }

    void load_ind_matrices(const std::string& filename, std::vector<std::vector<int>> &transition_ind,
                           std::vector<std::vector<int>> &emission_ind,
                           std::vector<int> &endProb_ind,
                           std::vector<int> &startingProb_ind) {
        std::ifstream infile(filename);
        if (!infile.is_open()) {
            std::cerr << "Could not open the file!" << std::endl;
            return;
        }

        std::string line;
        std::string section;

        while (getline(infile, line)) {
            // Trim leading and trailing whitespace
            line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));
            line.erase(line.find_last_not_of(" \t\n\r\f\v") + 1);

            if (line.empty()) continue;

            if (line == "Transition Matrix:") {
                section = "transition";
            } else if (line == "End Probabilities:") {
                section = "end_prob";
            } else if (line == "Starting Probabilities:") {
                section = "starting_prob";
            } else if (line == "Emission Matrix:") {
                section = "emission";
            } else {
                std::istringstream iss(line);
                int from_state, to_state, n, shrg_index;

                if (section == "transition") {
                    iss >> from_state >> to_state >> shrg_index;
                    if (from_state >= transition_ind.size()) {
                        transition_ind.resize(from_state + 1);
                    }
                    if (to_state >= transition_ind[from_state].size()) {
                        transition_ind[from_state].resize(to_state + 1, -1);
                    }
                    transition_ind[from_state][to_state] = shrg_index;
                } else if (section == "end_prob") {
                    iss >> from_state >> shrg_index;
                    if (from_state >= endProb_ind.size()) {
                        endProb_ind.resize(from_state + 1, -1);
                    }
                    endProb_ind[from_state] = shrg_index;
                } else if (section == "starting_prob") {
                    iss >> from_state >> shrg_index;
                    if (from_state >= startingProb_ind.size()) {
                        startingProb_ind.resize(from_state + 1, -1);
                    }
                    startingProb_ind[from_state] = shrg_index;
                } else if (section == "emission") {
                    iss >> from_state >> n >> shrg_index;
                    if (from_state >= emission_ind.size()) {
                        emission_ind.resize(from_state + 1);
                    }
                    if (n >= emission_ind[from_state].size()) {
                        emission_ind[from_state].resize(n + 1, -1);
                    }
                    emission_ind[from_state][n] = shrg_index;
                }
            }
        }

        infile.close();
    }
}