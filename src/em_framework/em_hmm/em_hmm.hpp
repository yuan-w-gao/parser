#pragma once
#include "em.hpp"
#include "em_hmm_utils.hpp"
#include "em.hpp"

namespace shrg{
namespace em{
class EM_HMM : public EM{
      public :
        EM_HMM(RuleVector &shrg_rules, vector<EdsGraph> &graphs, Context *context, double threshold, string dir, const string& validationDir);
        void run() override;

//      protected:
//        bool converged() const override;
//        void computeExpectedCount(ChartItem *root, double pw) override;
//        void updateEM() override;

      private:
        int num_states;
        int num_symbols;
        vector<vector<int>> transition_ind;
        vector<int> endProb_ind;
        vector<int> startingProb_ind;
        vector<vector<int>> emission_ind;

        vector<vector<double>> transition_true;
        vector<double> endProb_true;
        vector<vector<double>> emission_true;

        vector<vector<double>> hmm_transition;
        vector<vector<double>> hmm_emission;

        void initialize_true(LabelToRule& dict);
        double evaluate_kl();
        double evaluate_cross_entropy();
        vector<vector<double>> ind_to_matrix(vector<vector<int>> transition, size_t n, size_t m);
        vector<double> ind_to_vector(vector<int> prob_ind);
        void log_matrices(const string& log_file);
        void print_matrix_norm();
        void log_iteration_info(int iteration, double ll, double entropy, double kl,double hmm_entroopy, double hmm_kl, double time_diff, const string& log_file);
    };
}
}

