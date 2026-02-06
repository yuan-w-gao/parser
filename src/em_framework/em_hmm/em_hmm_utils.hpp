//
// Created by Yuan Gao on 24/06/2024.
//
#pragma once
#include <vector>
#include <fstream>
#include <random>
#include "em_types.hpp"

namespace shrg::em{
void log_matrix_to_file(const std::vector<std::vector<double>>& m, std::ofstream& log_stream);
void log_vector_to_file(const std::vector<double>& vec, std::ofstream& log_stream);
void print_vector(std::vector<double> p);
void print_matrix(std::vector<std::vector<double>> m);

double add_logs(double log_a, double log_b);
void normalize_weights(RuleVector& rules);
std::vector<double> normalize_log_probs(const std::vector<double>& log_probs);
std::vector<double> normalize_probs(const std::vector<double>& probs);

double frobenius_norm(const std::vector<std::vector<double>>& matrix);
double generate_gaussian_noise(double mean, double stddev);
double cross_entropy_log_q(const std::vector<double>& p, const std::vector<double>& log_q);


void load_HMM_parameters(const std::string& filename, int num_states, int num_symbols,
                         std::vector<std::vector<double>>& A,
                         std::vector<std::vector<double>>& B);
void load_true_prob(const std::string& filename, std::vector<std::vector<double>> &transition_true,
                    std::vector<std::vector<double>> &emission_true,
                    std::vector<double> &end_prob_true);
void load_ind_matrices(const std::string& filename, std::vector<std::vector<int>> &transition_ind,
                       std::vector<std::vector<int>> &emission_ind,
                       std::vector<int> &endProb_ind,
                       std::vector<int> &startingProb_ind);

}
