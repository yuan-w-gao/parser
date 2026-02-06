//
// Created by Yuan Gao on 22/09/2025.
// Main EM experiment runner - supports all EM algorithms
//

#include "experiment_runner/experiment_runner.hpp"
#include <iostream>
#include <string>
#include <vector>

using namespace shrg::experiment;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <algorithm> <experiment_directory> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Algorithms:" << std::endl;
    std::cout << "  batch_em     - Batch EM algorithm" << std::endl;
    std::cout << "  viterbi_em   - Viterbi EM algorithm" << std::endl;
    std::cout << "  online_em    - Online EM algorithm" << std::endl;
    std::cout << "  full_em      - Full EM algorithm" << std::endl;
    std::cout << "  all          - Run all algorithms" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --batch-size <size>      Batch size for batch EM (default: 5)" << std::endl;
    std::cout << "  --max-iterations <num>   Maximum iterations (default: 50)" << std::endl;
    std::cout << "  --threshold <value>      Convergence threshold (default: 0.01)" << std::endl;
    std::cout << "  --parser-type <type>     Parser type (default: tree_v2)" << std::endl;
    std::cout << "  --timeout <seconds>      Timeout in seconds (default: 300)" << std::endl;
    std::cout << "  --no-evaluation          Skip evaluation step" << std::endl;
    std::cout << "  --quiet                  Reduce output verbosity" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " batch_em /path/to/incremental/.../by_ages/10" << std::endl;
    std::cout << "  " << program_name << " all /path/to/incremental/.../by_ages/10 --batch-size 10" << std::endl;
}

bool parseArgs(int argc, char* argv[], std::string& algorithm, ExperimentConfig& config) {
    if (argc < 3) {
        return false;
    }

    algorithm = argv[1];
    config.experiment_dir = argv[2];

    // Parse optional arguments
    for (int i = 3; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--batch-size" && i + 1 < argc) {
            config.batch_size = std::stoi(argv[++i]);
        } else if (arg == "--max-iterations" && i + 1 < argc) {
            config.max_iterations = std::stoi(argv[++i]);
        } else if (arg == "--threshold" && i + 1 < argc) {
            config.threshold = std::stod(argv[++i]);
        } else if (arg == "--parser-type" && i + 1 < argc) {
            config.parser_type = argv[++i];
        } else if (arg == "--timeout" && i + 1 < argc) {
            config.timeout_seconds = std::stoi(argv[++i]);
        } else if (arg == "--no-evaluation") {
            config.run_evaluation = false;
        } else if (arg == "--quiet") {
            // Will be handled in main
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            return false;
        }
    }

    return true;
}

bool isValidAlgorithm(const std::string& algorithm) {
    return algorithm == "batch_em" || algorithm == "viterbi_em" ||
           algorithm == "online_em" || algorithm == "full_em" || algorithm == "all";
}

int main(int argc, char* argv[]) {
    std::string algorithm;
    ExperimentConfig config;

    // Parse command line arguments
    if (!parseArgs(argc, argv, algorithm, config)) {
        printUsage(argv[0]);
        return 1;
    }

    // Validate algorithm
    if (!isValidAlgorithm(algorithm)) {
        std::cerr << "Error: Invalid algorithm '" << algorithm << "'" << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    // Validate configuration
    if (!config.isValid()) {
        std::cerr << "Error: Invalid experiment configuration" << std::endl;
        std::cerr << "Please check that the following files exist:" << std::endl;
        std::cerr << "  Grammar: " << config.getGrammarFile() << std::endl;
        std::cerr << "  Graphs:  " << config.getGraphFile() << std::endl;
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

    // Print experiment info
    if (verbose) {
        std::cout << "=== EM Experiment Runner ===" << std::endl;
        std::cout << "Algorithm: " << algorithm << std::endl;
        std::cout << "Experiment directory: " << config.experiment_dir << std::endl;
        std::cout << "Grammar file: " << config.getGrammarFile() << std::endl;
        std::cout << "Graph file: " << config.getGraphFile() << std::endl;
        std::cout << "Output directory: " << config.getOutputDir() << std::endl;
        std::cout << "Parser type: " << config.parser_type << std::endl;
        std::cout << "Batch size: " << config.batch_size << std::endl;
        std::cout << "Max iterations: " << config.max_iterations << std::endl;
        std::cout << "Threshold: " << config.threshold << std::endl;
        std::cout << "Timeout: " << config.timeout_seconds << "s" << std::endl;
        std::cout << "Run evaluation: " << (config.run_evaluation ? "yes" : "no") << std::endl;
        std::cout << "=============================" << std::endl;
    }

    // Create and run experiment
    ExperimentRunner runner(config);
    runner.setVerbose(verbose);

    bool success = false;

    if (algorithm == "all") {
        success = runner.runAllAlgorithms();
    } else {
        try {
            AlgorithmType algo_type = stringToAlgorithmType(algorithm);
            success = runner.runExperiment(algo_type);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            return 1;
        }
    }

    if (success) {
        if (verbose) {
            std::cout << "Experiment completed successfully!" << std::endl;
            std::cout << "Results saved to: " << config.getOutputDir() << std::endl;
        }
        return 0;
    } else {
        std::cerr << "Experiment failed!" << std::endl;
        return 1;
    }
}