//
// Created by Yuan Gao on 24/06/2024.
//
#pragma once

#include "em_base.hpp"

namespace shrg::em{
    class HMM_Base{
      public:
        HMM_Base(RuleVector &shrg_rules, std::vector<EdsGraph> &graphs, Context *context,
                 double threshold, std::string dir, const std::string& validationDir);
        void run();
      protected:
        EMBase *em_algo;
      private:
        int num_states;
        int num_symbols;
        std::vector<std::vector<int>> transition_ind;
        std::vector<int> endProb_ind;
        std::vector<int> startingProb_ind;
        std::vector<std::vector<int>> emission_ind;

        std::vector<std::vector<double>> transition_true;
        std::vector<double> endProb_true;
        std::vector<std::vector<double>> emission_true;

        std::vector<std::vector<double>> hmm_transition;
        std::vector<std::vector<double>> hmm_emission;

        void initialize_true(LabelToRule& dict);
        double evaluate_kl();
        double evaluate_cross_entropy();
        std::vector<std::vector<double>> ind_to_matrix(std::vector<std::vector<int>> transition, size_t n, size_t m);
        std::vector<double> ind_to_vector(std::vector<int> prob_ind);
        void log_matrices(const std::string& log_file);
        void print_matrix_norm();
        void log_iteration_info(int iteration, double ll, double entropy, double kl,double hmm_entroopy, double hmm_kl, double time_diff, const std::string& log_file);
    };
}