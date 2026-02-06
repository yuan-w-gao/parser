//
// Created by Yuan Gao on 22/09/2025.
// Comprehensive evaluation runner for EM experiments
//

#include "evaluation_framework/evaluation_runner.hpp"
#include <iostream>

using namespace shrg::evaluation;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <experiment_directory> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Description:" << std::endl;
    std::cout << "  Run comprehensive evaluation on EM experiment results." << std::endl;
    std::cout << "  Evaluates parsing accuracy, generation quality, and tree structure." << std::endl;
    std::cout << "  Integrates with Python evaluation scripts from fla/src/." << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --no-parsing-accuracy    Skip parsing accuracy evaluation" << std::endl;
    std::cout << "  --no-generation-bleu     Skip generation BLEU evaluation" << std::endl;
    std::cout << "  --no-tree-eval           Skip tree structure evaluation" << std::endl;
    std::cout << "  --no-f1-eval             Skip F1 score evaluation" << std::endl;
    std::cout << "  --no-python-eval         Skip Python evaluation integration" << std::endl;
    std::cout << "  --no-comparison-report   Skip algorithm comparison report" << std::endl;
    std::cout << "  --quiet                  Reduce output verbosity" << std::endl;
    std::cout << "  --detailed-results       Save detailed per-sample results" << std::endl;
    std::cout << std::endl;
    std::cout << "Requirements:" << std::endl;
    std::cout << "  - Experiment directory must contain induced_outputs/ with derivation files" << std::endl;
    std::cout << "  - Gold standard derivations should be in for_induction/train.derivations.p" << std::endl;
    std::cout << "  - Python environment with required packages for fla/ integration" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " /path/to/incremental/.../by_ages/10" << std::endl;
    std::cout << "  " << program_name << " /path/to/experiment --no-python-eval --detailed-results" << std::endl;
}

bool parseArgs(int argc, char* argv[], EvaluationConfig& config) {
    if (argc < 2) {
        return false;
    }

    config.experiment_dir = argv[1];
    config.setupDefaultPaths();

    // Parse optional arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--no-parsing-accuracy") {
            config.run_parsing_accuracy = false;
        } else if (arg == "--no-generation-bleu") {
            config.run_generation_bleu = false;
        } else if (arg == "--no-tree-eval") {
            config.run_derivation_tree_eval = false;
        } else if (arg == "--no-f1-eval") {
            config.run_f1_evaluation = false;
        } else if (arg == "--no-python-eval") {
            config.run_python_evaluation = false;
        } else if (arg == "--no-comparison-report") {
            config.create_comparison_report = false;
        } else if (arg == "--quiet") {
            // Will be handled in main
        } else if (arg == "--detailed-results") {
            config.save_detailed_results = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return false;
        }
    }

    return true;
}

void printExperimentInfo(const EvaluationConfig& config) {
    std::cout << "=== Comprehensive Evaluation ===" << std::endl;
    std::cout << "Experiment directory: " << config.experiment_dir << std::endl;
    std::cout << "Experiment name: " << config.getExperimentName() << std::endl;
    std::cout << "Output directory: " << config.induced_outputs_dir << std::endl;

    if (config.hasGoldStandard()) {
        std::cout << "Gold standard: Available" << std::endl;
        if (!config.gold_derivations_file.empty()) {
            std::cout << "  Pickle file: " << config.gold_derivations_file << std::endl;
        }
        if (!config.gold_deriv_txt_file.empty()) {
            std::cout << "  Text file: " << config.gold_deriv_txt_file << std::endl;
        }
    } else {
        std::cout << "Gold standard: Not found (some evaluations will be skipped)" << std::endl;
    }

    std::cout << "Available algorithms: ";
    const auto& algorithms = config.getAvailableAlgorithms();
    for (size_t i = 0; i < algorithms.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << algorithms[i];
    }
    std::cout << std::endl;

    std::cout << "Evaluation types:" << std::endl;
    std::cout << "  Parsing accuracy: " << (config.run_parsing_accuracy ? "yes" : "no") << std::endl;
    std::cout << "  Generation BLEU: " << (config.run_generation_bleu ? "yes" : "no") << std::endl;
    std::cout << "  Tree structure: " << (config.run_derivation_tree_eval ? "yes" : "no") << std::endl;
    std::cout << "  F1 evaluation: " << (config.run_f1_evaluation ? "yes" : "no") << std::endl;
    std::cout << "  Python integration: " << (config.run_python_evaluation ? "yes" : "no") << std::endl;

    std::cout << "=================================" << std::endl;
}

int main(int argc, char* argv[]) {
    EvaluationConfig config;

    // Parse command line arguments
    if (!parseArgs(argc, argv, config)) {
        printUsage(argv[0]);
        return 1;
    }

    // Validate configuration
    if (!config.isValid()) {
        std::cerr << "Error: Invalid experiment configuration" << std::endl;
        std::cerr << "Please check that the following exist:" << std::endl;
        std::cerr << "  Experiment directory: " << config.experiment_dir << std::endl;
        std::cerr << "  Induced outputs: " << config.induced_outputs_dir << std::endl;

        if (config.getAvailableAlgorithms().empty()) {
            std::cerr << "  No algorithm result files found in induced_outputs/" << std::endl;
            std::cerr << "  Expected files: *_derivations.txt" << std::endl;
        }

        return 1;
    }

    // Check for quiet mode
    bool verbose = true;
    for (int i = 1; i < argc; i++) {
        if (std::string(argv[i]) == "--quiet") {
            verbose = false;
            break;
        }
    }

    // Print experiment information
    if (verbose) {
        printExperimentInfo(config);
    }

    // Create and run evaluation
    EvaluationRunner runner(config);
    runner.setVerbose(verbose);

    auto start_time = std::chrono::steady_clock::now();

    bool success = runner.runCompleteEvaluation();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);

    if (success) {
        if (verbose) {
            std::cout << "Comprehensive evaluation completed successfully in "
                      << duration.count() << " seconds!" << std::endl;
            std::cout << "Results available in: " << config.induced_outputs_dir << std::endl;
            std::cout << std::endl;
            std::cout << "Key output files:" << std::endl;
            std::cout << "  - evaluation_summary.json (overall summary)" << std::endl;
            std::cout << "  - algorithm_comparison.json (algorithm comparison)" << std::endl;
            std::cout << "  - *_evaluation_detailed.json (per-algorithm details)" << std::endl;
            std::cout << "  - *_parsing_accuracy.json (parsing accuracy results)" << std::endl;
        }
        return 0;
    } else {
        std::cerr << "Comprehensive evaluation failed!" << std::endl;
        return 1;
    }
}