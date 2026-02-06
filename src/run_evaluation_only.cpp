//
// Created by Yuan Gao on 22/09/2025.
// Run evaluation only on existing experiment results
//

#include "experiment_runner/experiment_runner.hpp"
#include <iostream>
#include <filesystem>

using namespace shrg::experiment;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <experiment_directory>" << std::endl;
    std::cout << std::endl;
    std::cout << "Description:" << std::endl;
    std::cout << "  Run evaluation on an experiment that has already completed EM training." << std::endl;
    std::cout << "  This will generate BLEU scores, F1 scores, and derivation comparisons." << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  experiment_directory    Path to experiment folder containing induced_outputs/" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " /path/to/incremental/.../by_ages/10" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string experiment_dir = argv[1];

    // Create experiment configuration
    ExperimentConfig config(experiment_dir);

    // Validate configuration
    if (!config.isValid()) {
        std::cerr << "Error: Invalid experiment configuration" << std::endl;
        std::cerr << "Please check that the following files exist:" << std::endl;
        std::cerr << "  Grammar: " << config.getGrammarFile() << std::endl;
        std::cerr << "  Graphs:  " << config.getGraphFile() << std::endl;
        return 1;
    }

    // Check if induced_outputs directory exists
    std::string output_dir = config.getOutputDir();
    if (!std::filesystem::exists(output_dir)) {
        std::cerr << "Error: Output directory does not exist: " << output_dir << std::endl;
        std::cerr << "Please run the EM experiment first." << std::endl;
        return 1;
    }

    std::cout << "=== Evaluation Only ===" << std::endl;
    std::cout << "Experiment directory: " << experiment_dir << std::endl;
    std::cout << "Output directory: " << output_dir << std::endl;
    std::cout << "=======================" << std::endl;

    // Create experiment runner
    ExperimentRunner runner(config);
    runner.setVerbose(true);

    // Setup experiment (load data)
    if (!runner.setupExperiment()) {
        std::cerr << "Failed to setup experiment" << std::endl;
        return 1;
    }

    // Check which algorithm results exist and run evaluation for each
    std::vector<std::string> algorithms = {"batch_em", "viterbi_em", "online_em", "full_em"};
    bool any_evaluations_run = false;

    for (const std::string& algorithm : algorithms) {
        // Check if this algorithm has saved results
        std::string deriv_file = config.getDerivationOutputFile(algorithm);
        std::string eval_file = config.getEvaluationOutputFile(algorithm);

        // Skip if derivation file doesn't exist or evaluation already exists
        if (!std::filesystem::exists(deriv_file)) {
            std::cout << "Skipping " << algorithm << " - no derivation file found" << std::endl;
            continue;
        }

        if (std::filesystem::exists(eval_file)) {
            std::cout << "Skipping " << algorithm << " - evaluation already exists" << std::endl;
            continue;
        }

        std::cout << "Running evaluation for: " << algorithm << std::endl;

        try {
            // Create algorithm instance and load from saved state if possible
            AlgorithmType algo_type = stringToAlgorithmType(algorithm);
            auto em_algorithm = runner.createAlgorithm(algo_type);

            if (em_algorithm) {
                if (runner.runEvaluation(em_algorithm.get(), algorithm)) {
                    std::cout << "Evaluation completed for: " << algorithm << std::endl;
                    any_evaluations_run = true;
                } else {
                    std::cerr << "Evaluation failed for: " << algorithm << std::endl;
                }
            } else {
                std::cerr << "Failed to create algorithm: " << algorithm << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error evaluating " << algorithm << ": " << e.what() << std::endl;
        }

        std::cout << std::endl;
    }

    if (!any_evaluations_run) {
        std::cout << "No evaluations were run. All algorithms may already have evaluation results." << std::endl;
        return 0;
    }

    std::cout << "Evaluation completed! Results saved to: " << output_dir << std::endl;
    return 0;
}