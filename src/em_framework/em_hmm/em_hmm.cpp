//
// Created by Yuan Gao on 19/06/2024.
//
#include "em_hmm.hpp"

#include <utility>
#include <random>
#include "em_types.hpp"
#include "em_utils.hpp"
#include "em.hpp"


namespace shrg::em{
    EM_HMM::EM_HMM(RuleVector &shrg_rules, std::vector<EdsGraph> &graphs, Context *context,
               double threshold, std::string dir, const std::string& validationDir)
    : EM(shrg_rules, graphs, context, threshold, dir) {
        transition_true = vector<vector<double>>{};
        endProb_true = vector<double>{};
        emission_true = vector<vector<double>>{};
        load_true_prob(validationDir+"true_distribution", transition_true, emission_true, endProb_true);

        num_states = transition_true.size();
        num_symbols = emission_true[0].size();

        transition_ind.resize(num_states, vector<int>(num_states, 0.0));
        endProb_ind.resize(num_states, 0.0);
	startingProb_ind.resize(num_states, 0.0);
        emission_ind.resize(num_states, vector<int>(num_symbols, 0.0));
        load_ind_matrices(validationDir+"state_to_shrgIndex.txt", transition_ind, emission_ind, endProb_ind, startingProb_ind);

        hmm_transition.resize(num_states, vector<double>(num_states, 0.0));
        hmm_emission.resize(num_states, vector<double>(num_symbols, 0.0));
        load_HMM_parameters(output_dir+"hmm_final", num_states, num_symbols, hmm_transition, hmm_emission);
    }

    void EM_HMM::log_matrices(const string& log_file) {
        ofstream log_stream(log_file, ios_base::app); // Append mode
        if (!log_stream) {
            cerr << "Error opening log file: " << log_file << endl;
            return;
        }

        vector<vector<double>> transitions = ind_to_matrix(transition_ind, transition_ind.size(), transition_ind[0].size());
        vector<vector<double>> emissions = ind_to_matrix(emission_ind, emission_true.size(), emission_true[0].size());
        vector<double> ends = ind_to_vector(endProb_ind);

        double transition_norm = frobenius_norm(transitions);
        double emission_norm = frobenius_norm(emissions);
        double transition_true_norm = frobenius_norm(transition_true);
        double emission_true_norm = frobenius_norm(emission_true);

        log_stream << "transition:\n";
        log_matrix_to_file(transitions, log_stream);
        log_stream << "true transition: \n";
        log_matrix_to_file(transition_true, log_stream);

        log_stream << "emissions: \n";
        log_matrix_to_file(emissions, log_stream);
        log_stream << "true emissions: \n";
        log_matrix_to_file(emission_true, log_stream);


        log_stream << "end prob and true end prob: \n";
        for (size_t i = 0; i < max(ends.size(), endProb_true.size()); ++i) {
            if (i < ends.size()) {
                log_stream << ends[i] << " ";
            } else {
                log_stream << "  "; // Print space if ends element is missing
            }

            log_stream << "\t"; // Separator

            if (i < endProb_true.size()) {
                log_stream << endProb_true[i] << " ";
            }

            log_stream << "\n";
        }

        log_stream << "transition_norm: " << transition_norm << ", true norm: " << transition_true_norm;
        log_stream << "\nemission_norm: " << emission_norm << ", true norm: " << emission_true_norm << "\n";
        log_stream << "\n\n";

        log_stream.close();
    }

    void EM_HMM::log_iteration_info(int iteration, double ll, double entropy, double kl,double hmm_entropy, double hmm_kl, double time_diff, const string& log_file) {
        ofstream log_stream(log_file, ios_base::app); // Append mode
        if (!log_stream) {
            cerr << "Error opening log file: " << log_file << endl;
            return;
        }
        log_stream << "iteration: " << iteration << "\nlog likelihood: " << ll;
        log_stream << ", entropy: " << entropy;
        log_stream << ", KL: " << kl;
        log_stream << ", hmm_entropy: " << hmm_entropy << ", hmm_kl: " << hmm_kl;
        log_stream << ", in " << time_diff << " seconds \n";
        log_stream.close();
        log_matrices(log_file);
    }

    double cross_entropy(const std::vector<double>& P, const std::vector<double>& Q) {
        if (P.size() != Q.size()) {
            throw std::invalid_argument("Vectors P and Q must have the same size.");
        }

        double cross_entropy = 0.0;
        for (size_t i = 0; i < P.size(); ++i) {
            if (P[i] > 0) {
                cross_entropy -= P[i] * std::log(Q[i]);
            }
        }
        return cross_entropy;
    }

    double sum_cross_entropy(vector<vector<double>> A, vector<vector<double>> B){
        double total_entropy = 0.0;
        for(size_t i = 0; i < A.size(); i++){
            const vector<double> &a = A[i];
            const vector<double> &b = B[i];
            total_entropy += cross_entropy(a, b);
        }
        return total_entropy;
    }
    double kl_divergence(const vector<double>& p, const vector<double>& q) {
        double divergence = 0.0;
        for (size_t i = 0; i < p.size(); ++i) {
            if (p[i] > 0 && q[i] > 0) {
                divergence += p[i] * log(p[i] / q[i]);
            }
        }
        return divergence;
    }

    double sum_kl(vector<vector<double>> A, vector<vector<double>> B){
        double total_kl = 0.0;
        for(size_t i = 0; i < A.size(); i++){
            const vector<double> &a = A[i];
            const vector<double> &b = B[i];
            total_kl += kl_divergence(a, b);
        }
        return total_kl;
    }

    void log_info(string filename,
                  int iteration, double ll, double entropy, double kl, double hmm_kl,
                  double hmm_entro, vector<vector<double>> transition, vector<vector<double>>
                                                                           emi,
                  vector<double> end){
        ofstream log_stream(filename, ios_base::app);
        if (!log_stream) {
            cerr << "Error opening log file: " << endl;
            return;
        }
        log_stream << "iteration: " << iteration << "\nlog likelihood: " << ll;
        log_stream << ", entropy: " << entropy;
        log_stream << ", KL: " << kl;
        log_stream << ", hmm_entropy: " << hmm_entro << ", hmm_kl: " << hmm_kl;
        log_stream << "\ntransition: \n";
        log_matrix_to_file(transition, log_stream);
        log_stream << "emission: \n";
        log_matrix_to_file(emi, log_stream);
        log_stream << "end: \n";
        log_vector_to_file(end, log_stream);
        log_stream.close();
    }

    void EM_HMM::run() {
        std::cout << "Training Time~ \n";

        ofstream log_stream(output_dir+"log", ios_base::out);
        log_stream.close();
        clock_t t1,t2;
        unsigned long training_size = graphs.size();
        int iteration = 0;
        ll = 0;
        initialize_true(rule_dict);

        std::vector<double> history[shrg_rules.size()];
        std::vector<double> history_graph_ll[training_size];
        for(int i = 0; i < shrg_rules.size(); i++){
            history[i].push_back(shrg_rules[i]->log_rule_weight);
        }
        std::vector<std::vector<std::vector<double>>> transitions;
        std::vector<std::vector<std::vector<double>>> emissions;
        std::vector<std::vector<double>> endprobs;

        Generator *generator = context->parser->GetGenerator();

        double kl, hmm_kl;
        double entropy, hmm_entropy;
        vector<vector<double>> transition = ind_to_matrix(transition_ind, num_states, num_states);
        vector<vector<double>> emission = ind_to_matrix(emission_ind, num_states, num_symbols);
        transitions.push_back(transition);
        emissions.push_back(emission);
        vector<double> end = ind_to_vector(endProb_ind);
        endprobs.push_back(end);
        vector<double> kls, entropys, hmm_kls, hmm_entropys;
        kls.push_back(evaluate_kl());
        entropys.push_back(evaluate_cross_entropy());
        hmm_kls.push_back(sum_kl(transition, transition_true));
        hmm_entropys.push_back(sum_cross_entropy(transition, transition_true));

        std::vector<double> lls;
        lls.push_back(ll);
        do{
            prev_ll = ll;
            ll = 0;
            t1 = clock();
            for(int i = 0; i < training_size; i++){
                //            if(i % 50 == 0){
                //                std::cout << i << "\n";
                //            }
                //            std::cout << i << "\n";
                EdsGraph graph = graphs[i];
                auto code = context->Parse(graph);
                if(code == ParserError::kNone) {
                    ChartItem *root = context->parser->Result();
                    addParentPointerOptimized(root, 0);
//                    addChildPointer(root, generator);
                    addRulePointer(root);

                    double pw = computeInside(root);
                    computeOutside(root);
                    computeExpectedCount(root, pw);

                    //                ll = addLogs(ll, pw);
                    ll += pw;
                    history_graph_ll[i].push_back(pw);
                }
            }
            updateEM();
            std::cout << "max change: " << max_change << ", ind: " << max_change_ind << "\n";
            for(int i = 0; i < shrg_rules.size(); i++){
                history[i].push_back(shrg_rules[i]->log_rule_weight);
            }
            clearRuleCount();
            transition = ind_to_matrix(transition_ind, transition_ind.size(), transition_ind[0].size());
            emission = ind_to_matrix(emission_ind, emission_ind.size(), emission_ind[0].size());
            hmm_entropy = sum_cross_entropy(transition, hmm_transition);
            hmm_kl = sum_kl(transition, hmm_transition);
            entropy = evaluate_cross_entropy();
            kl = evaluate_kl();
            transitions.push_back(transition);
            emissions.push_back(emission);
            hmm_entropys.push_back(hmm_entropy);
            hmm_kls.push_back(hmm_kl);
            entropys.push_back(entropy);
            kls.push_back(kl);
            end = ind_to_vector(endProb_ind);
            endprobs.push_back(end);
            t2 = clock();
            double time_diff = (double)(t2 - t1)/CLOCKS_PER_SEC;
//            log_iteration_info(iteration, ll, entropy, kl, hmm_entropy, hmm_kl,time_diff, output_dir+"log");
//            std::cout << "iteration: " << iteration << "\nlog likelihood: " << ll;
//            std::cout << ", entropy: " << entropy;
//            std::cout << ", KL: " << kl;
//            std::cout << ", in " << time_diff << " seconds \n\n";
//            print_matrix_norm();
            lls.push_back(ll);
            if(! (output_dir == "N")){
                writeHistoryToDir(output_dir, history, shrg_rules.size());
                writeGraphLLToDir(output_dir, history_graph_ll, training_size);
                writeLLToDir(output_dir, lls, iteration);
            }

            iteration++;
        }while(!converged());

//        ofstream s(output_dir+"log", ios_base::app);
        for(size_t i = 0; i < transitions.size(); i++){
            log_info(output_dir+"log", i, lls[i], entropys[i], kls[i], hmm_entropys[i], hmm_kls[i],
                     transitions[i], emissions[i], endprobs[i]);
        }

        std::cout << "finished";
    }



    void EM_HMM::initialize_true(LabelToRule& dict) {

        for (size_t from_state = 0; from_state < transition_ind.size(); ++from_state) {
            for (size_t to_state = 0; to_state < transition_ind[from_state].size(); ++to_state) {
                int rule_index = transition_ind[from_state][to_state];
                if (rule_index != -1) {
                    double true_prob = transition_true[from_state][to_state];
                    double noisy_prob = true_prob + generate_gaussian_noise(0.0, 0.1);  // Mean = 0.0, Stddev = 0.1

                    if (noisy_prob < 0.0) {
                        noisy_prob = 1e-10;
                    }

                    shrg_rules[rule_index]->log_rule_weight = log(noisy_prob);
                }
            }
        }

        for (size_t state = 0; state < emission_ind.size(); ++state) {
            for (size_t symbol = 0; symbol < emission_ind[state].size(); ++symbol) {
                int rule_index = emission_ind[state][symbol];
                if (rule_index != -1) {
                    double true_prob = emission_true[state][symbol];
                    double noisy_prob = true_prob + generate_gaussian_noise(0.0, 0.1);

                    if (noisy_prob < 0.0) {
                        noisy_prob = 1e-10;
                    }

                    shrg_rules[rule_index]->log_rule_weight = log(noisy_prob);
                }
            }
        }

        for (auto& pair : dict) {
            normalize_weights(pair.second);
        }
    }

    int letterToIndex(char letter) {
        return letter - 'a';
    }

    vector<vector<double>> EM_HMM::ind_to_matrix(const vector<vector<int>> transition, size_t n, size_t m){
        vector<vector<double>> res = {};
        for(size_t i = 0; i < n; i ++){
            vector<double> curr = {};
//            vector<double> curr(transition[i].size());
            for(size_t j = 0; j < m; j++){
                int ind = transition[i][j];
                if(ind == -1){
                    curr.push_back(0.0);
//                    continue;
                }else{
//                    curr[j] = exp(shrg_rules[ind]->log_rule_weight);
                    curr.push_back(exp(shrg_rules[ind]->log_rule_weight));
                }

            }
            res.push_back(curr);
        }
        return res;
    }


    vector<double> EM_HMM::ind_to_vector(vector<int> prob_ind){
        vector<double> res (prob_ind.size());
        for(size_t i = 0; i < prob_ind.size(); i ++){
            int ind = prob_ind[i];
            if(ind == -1){
                res[i] = 0.0;
            }else{
                res[i] = exp(shrg_rules[ind]->log_rule_weight);
//                res.push_back(shrg_rules[ind]->log_rule_weight);
            }
        }
        return res;
    }

    void EM_HMM::print_matrix_norm(){
        vector<vector<double>> transitions = ind_to_matrix(transition_ind, transition_ind.size(), transition_ind[0].size());
        vector<vector<double>> emissions = ind_to_matrix(emission_ind, emission_true.size(), emission_true[0].size());

        vector<double> ends = ind_to_vector(endProb_ind);

        double transition_norm = frobenius_norm(transitions);
        double emission_norm = frobenius_norm(emissions);
        double transition_true_norm = frobenius_norm(transition_true);
        double emission_true_norm = frobenius_norm(emission_true);

        std::cout << "transition: \n";
        print_matrix(transitions);
        std::cout << "true transition: \n";
        print_matrix(transition_true);
        std::cout << "emissions: \n";
        print_matrix(emissions);
        std::cout << "true emission\n";
        print_matrix(emission_true);

        std::cout << "end prob: \n";
        print_vector(ends);
        std::cout << "true end prob: \n";
        print_vector(endProb_true);


        std::cout << "transition_norm: " << transition_norm << ", true norm: " << transition_true_norm;
        std::cout << "\nemission_norm: " << emission_norm << ", true norm: " << emission_true_norm << "\n";
    }

    double EM_HMM::evaluate_cross_entropy() {
        double total_cross_entropy = 0.0;

        for (size_t from_state = 0; from_state < transition_true.size(); ++from_state) {
            const vector<double>& true_probs = transition_true[from_state];
            vector<double> log_estimated_probs(true_probs.size(), -numeric_limits<double>::infinity());

            for (size_t to_state = 0; to_state < true_probs.size(); ++to_state) {
                int rule_index = transition_ind[from_state][to_state];
                if (rule_index != -1) {
                    log_estimated_probs[to_state] = shrg_rules[rule_index]->log_rule_weight;
                }
            }

            total_cross_entropy += cross_entropy(normalize_probs(true_probs), log_estimated_probs);
        }

        for (size_t state = 0; state < emission_true.size(); ++state) {
            const vector<double>& true_probs = emission_true[state];
            vector<double> log_estimated_probs(true_probs.size(), -numeric_limits<double>::infinity());

            for (size_t symbol = 0; symbol < true_probs.size(); ++symbol) {
                int rule_index = emission_ind[state][symbol];
                if (rule_index != -1) {
                    log_estimated_probs[symbol] = shrg_rules[rule_index]->log_rule_weight;
                }
            }

            total_cross_entropy += cross_entropy(normalize_probs(true_probs), log_estimated_probs);
        }

        return total_cross_entropy;
    }

    double EM_HMM::evaluate_kl() {
        double total_kl = 0.0;

        for (size_t from_state = 0; from_state < transition_true.size(); ++from_state) {
            const vector<double>& true_probs = transition_true[from_state];
            vector<double> estimated_probs(true_probs.size(), 0.0);

            for (size_t to_state = 0; to_state < true_probs.size(); ++to_state) {
                int rule_index = transition_ind[from_state][to_state];
                if (rule_index != -1) {
                    estimated_probs[to_state] = exp(shrg_rules[rule_index]->log_rule_weight);
                }
            }

            total_kl += kl_divergence(true_probs, estimated_probs);
        }

        for (size_t state = 0; state < emission_true.size(); ++state) {
            const vector<double>& true_probs = emission_true[state];
            vector<double> estimated_probs(true_probs.size(), 0.0);

            for (size_t symbol = 0; symbol < true_probs.size(); ++symbol) {
                int rule_index = emission_ind[state][symbol];
                if (rule_index != -1) {
                    estimated_probs[symbol] = exp(shrg_rules[rule_index]->log_rule_weight);
                }
            }

            total_kl += kl_divergence(true_probs, estimated_probs);
        }

        return total_kl;
    }

//    void EM_HMM::loadHMMParameters(const std::string& filename, int num_states, int num_symbols,
//                           std::vector<std::vector<double>>& A,
//                           std::vector<std::vector<double>>& B) {
//        std::ifstream file(filename);
//        std::string line;
//        bool reading_transition = false;
//        bool reading_emission = false;
//        int current_state = 0;
//
//        if (!file.is_open()) {
//            std::cerr << "Failed to open the file." << std::endl;
//            return;
//        }
//
//        while (std::getline(file, line)) {
//            if (line.find("Transition probabilities:") != std::string::npos) {
//                reading_transition = true;
//                reading_emission = false;
//                current_state = 0;
//                continue;
//            }
//            if (line.find("Emission probabilities:") != std::string::npos) {
//                reading_transition = false;
//                reading_emission = true;
//                current_state = 0;
//                continue;
//            }
//            if (reading_transition && current_state < num_states) {
//                std::istringstream iss(line);
//                double value;
//                int symbol_index = 0;
//                while (iss >> value) {
//                    A[current_state][symbol_index] = value;
//                    symbol_index++;
//                }
//                current_state++;
//            }
//            if (reading_emission && current_state < num_states) {
//                std::istringstream iss(line);
//                double value;
//                int symbol_index = 0;
//                while (iss >> value) {
//                    B[current_state][symbol_index] = value;
//                    symbol_index++;
//                }
//                current_state++;
//            }
//        }
            //        void EM_HMM::loadHMMParameters(const std::string& filename, int num_states, int num_symbols,
//                                       std::vector<std::vector<double>>& A,
//                                       std::vector<std::vector<double>>& B) {
//            std::ifstream file(filename);
//            std::string line;
//            bool reading_transition = false;
//            bool reading_emission = false;
//            int current_state = 0;
//
//            if (!file.is_open()) {
//                std::cerr << "Failed to open the file." << std::endl;
//                return;
//            }
//
//            while (std::getline(file, line)) {
//                if (line.find("Transition probabilities:") != std::string::npos) {
//                    reading_transition = true;
//                    reading_emission = false;
//                    current_state = 0;
//                    continue;
//                }
//                if (line.find("Emission probabilities:") != std::string::npos) {
//                    reading_transition = false;
//                    reading_emission = true;
//                    current_state = 0;
//                    continue;
//                }
//                if (reading_transition && current_state < num_states) {
//                    std::istringstream iss(line);
//                    double value;
//                    int symbol_index = 0;
//                    while (iss >> value) {
//                        A[current_state][symbol_index] = value;
//                        symbol_index++;
//                    }
//                    current_state++;
//                }
//                if (reading_emission && current_state < num_states) {
//                    std::istringstream iss(line);
//                    double value;
//                    int symbol_index = 0;
//                    while (iss >> value) {
//                        B[current_state][symbol_index] = value;
//                        symbol_index++;
//                    }
//                    current_state++;
//                }
//            }
//
//            file.close();
//        }
//
//        file.close();
//    }

//    void EM_HMM::loadTrueProb(const string& filename) {
//        ifstream file(filename);
//        if (!file.is_open()) {
//            cerr << "Could not open the file!" << endl;
//            return;
//        }
//
//        string line;
//        enum Section {
//            NONE,
//            TRANSITION_MATRIX,
//            END_PROBABILITIES,
//            OMISSION_MATRIX
//        } currentSection = NONE;
//
//        while (getline(file, line)) {
//            if (line.empty()) {
//                continue; // Skip empty lines
//            }
//
//            if (line == "Transition Matrix:") {
//                currentSection = TRANSITION_MATRIX;
//                continue;
//            } else if (line == "End Probabilities:") {
//                currentSection = END_PROBABILITIES;
//                continue;
//            } else if (line == "Omission Matrix:") {
//                currentSection = OMISSION_MATRIX;
//                continue;
//            }
//
//            stringstream ss(line);
//            if (currentSection == TRANSITION_MATRIX) {
//                vector<double> row;
//                double value;
//                while (ss >> value) {
//                    row.push_back(value);
//                }
//                transition_true.push_back(row);
//            } else if (currentSection == END_PROBABILITIES) {
//                double value;
//                while (ss >> value) {
//                    endProb_true.push_back(value);
//                }
//            } else if (currentSection == OMISSION_MATRIX) {
//                vector<double> row;
//                double value;
//                while (ss >> value) {
//                    row.push_back(value);
//                }
//                emission_true.push_back(row);
//            }
//        }
//
//        file.close();
//    }
}
