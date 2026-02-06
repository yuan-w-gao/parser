//
// EM V2 Coordinator Runner - High-level experiment management
// Created by Yuan Gao on 22/09/2025.
//

#include "em_framework_v2/em_coordinator.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>

using namespace shrg::em::v2;

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <experiment_directory> [options]\n";
    std::cout << "\nOptions:\n";
    std::cout << "  --algorithms ALGO1,ALGO2,...  Algorithms to run (default: all)\n";
    std::cout << "                                Available: full_em,batch_em,viterbi_em,online_em\n";
    std::cout << "  --max-iterations N            Maximum iterations for all algorithms (default: 100)\n";
    std::cout << "  --convergence-threshold T     Convergence threshold (default: 0.001)\n";
    std::cout << "  --batch-size N                Batch size for batch_em (default: 5)\n";
    std::cout << "  --learning-rate R             Learning rate for online_em (default: 0.1)\n";
    std::cout << "  --timeout SECONDS             Timeout in seconds (default: 0 = no timeout)\n";
    std::cout << "  --verbose                     Enable verbose output\n";
    std::cout << "  --no-evaluation               Skip evaluation\n";
    std::cout << "  --no-derivations              Don't save derivations\n";
    std::cout << "  --output-dir DIR              Output directory for results\n";
    std::cout << "  --sequential                  Run algorithms sequentially (default)\n";
    std::cout << "  --parallel N                  Run algorithms in parallel (max N threads)\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << program_name << " /path/to/experiment --algorithms batch_em,viterbi_em --verbose\n";
    std::cout << "  " << program_name << " /path/to/experiment --algorithms all --max-iterations 50\n";
}

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

EMAlgorithmFactory::AlgorithmType stringToAlgorithmType(const std::string& str) {
    if (str == "full_em") return EMAlgorithmFactory::AlgorithmType::FULL_EM;
    if (str == "batch_em") return EMAlgorithmFactory::AlgorithmType::BATCH_EM;
    if (str == "viterbi_em") return EMAlgorithmFactory::AlgorithmType::VITERBI_EM;
    if (str == "online_em") return EMAlgorithmFactory::AlgorithmType::ONLINE_EM;
    throw std::invalid_argument("Unknown algorithm: " + str);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string experiment_dir = argv[1];

    // Setup experiment configuration with defaults
    EMExperimentConfig config;
    config.experiment_directory = experiment_dir;
    config.run_evaluation = true;
    config.save_derivations = true;
    config.compare_algorithms = true;
    config.run_algorithms_sequentially = true;
    config.max_parallel_algorithms = 1;

    // Default algorithms (all)
    config.algorithms_to_run = {
        EMAlgorithmFactory::AlgorithmType::FULL_EM,
        EMAlgorithmFactory::AlgorithmType::BATCH_EM,
        EMAlgorithmFactory::AlgorithmType::VITERBI_EM,
        EMAlgorithmFactory::AlgorithmType::ONLINE_EM
    };

    // Parse command line options
    for (int i = 2; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--algorithms" && i + 1 < argc) {
            std::string algorithms_str = argv[++i];
            config.algorithms_to_run.clear();

            if (algorithms_str == "all") {
                config.algorithms_to_run = {
                    EMAlgorithmFactory::AlgorithmType::FULL_EM,
                    EMAlgorithmFactory::AlgorithmType::BATCH_EM,
                    EMAlgorithmFactory::AlgorithmType::VITERBI_EM,
                    EMAlgorithmFactory::AlgorithmType::ONLINE_EM
                };
            } else {
                auto algo_names = split(algorithms_str, ',');
                for (const auto& algo_name : algo_names) {
                    try {
                        config.algorithms_to_run.push_back(stringToAlgorithmType(algo_name));
                    } catch (const std::exception& e) {
                        std::cerr << "Error: " << e.what() << std::endl;
                        return 1;
                    }
                }
            }
        } else if (arg == "--max-iterations" && i + 1 < argc) {
            int max_iter = std::stoi(argv[++i]);
            for (auto algo_type : config.algorithms_to_run) {
                config.algorithm_configs[algo_type].max_iterations = max_iter;
            }
        } else if (arg == "--convergence-threshold" && i + 1 < argc) {
            double threshold = std::stod(argv[++i]);
            for (auto algo_type : config.algorithms_to_run) {
                config.algorithm_configs[algo_type].convergence_threshold = threshold;
            }
        } else if (arg == "--batch-size" && i + 1 < argc) {
            int batch_size = std::stoi(argv[++i]);
            config.algorithm_configs[EMAlgorithmFactory::AlgorithmType::BATCH_EM].batch_size = batch_size;
        } else if (arg == "--learning-rate" && i + 1 < argc) {
            double lr = std::stod(argv[++i]);
            config.algorithm_configs[EMAlgorithmFactory::AlgorithmType::ONLINE_EM].learning_rate = lr;
        } else if (arg == "--timeout" && i + 1 < argc) {
            int timeout = std::stoi(argv[++i]);
            for (auto algo_type : config.algorithms_to_run) {
                config.algorithm_configs[algo_type].timeout_seconds = timeout;
            }
        } else if (arg == "--verbose") {
            for (auto algo_type : config.algorithms_to_run) {
                config.algorithm_configs[algo_type].verbose = true;
            }
        } else if (arg == "--no-evaluation") {
            config.run_evaluation = false;
        } else if (arg == "--no-derivations") {
            config.save_derivations = false;
        } else if (arg == "--output-dir" && i + 1 < argc) {
            config.output_directory = argv[++i];
        } else if (arg == "--sequential") {
            config.run_algorithms_sequentially = true;
            config.max_parallel_algorithms = 1;
        } else if (arg == "--parallel" && i + 1 < argc) {
            config.run_algorithms_sequentially = false;
            config.max_parallel_algorithms = std::stoi(argv[++i]);
        } else {
            std::cerr << "Warning: Unknown option '" << arg << "'\n";
        }
    }

    // Set output directory if not specified
    if (config.output_directory.empty()) {
        config.output_directory = experiment_dir + "/em_v2_results";
    }

    try {
        std::cout << "=== EM V2 Coordinator - Multi-Algorithm Experiment ===\n";
        std::cout << "Experiment directory: " << experiment_dir << "\n";
        std::cout << "Output directory: " << config.output_directory << "\n";
        std::cout << "Algorithms to run: ";
        for (size_t i = 0; i < config.algorithms_to_run.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << EMAlgorithmFactory::algorithmTypeToString(config.algorithms_to_run[i]);
        }
        std::cout << "\n";
        std::cout << "Evaluation enabled: " << (config.run_evaluation ? "YES" : "NO") << "\n";
        std::cout << "Save derivations: " << (config.save_derivations ? "YES" : "NO") << "\n";
        std::cout << "Execution mode: " << (config.run_algorithms_sequentially ? "Sequential" :
                                           "Parallel (" + std::to_string(config.max_parallel_algorithms) + " threads)") << "\n";
        std::cout << "======================================================\n";

        // Create coordinator
        EMCoordinator coordinator(config);

        // Load experiment data
        std::cout << "Loading experiment data...\n";
        if (!coordinator.initializeFromDirectory(experiment_dir)) {
            std::cerr << "Error: Could not load experiment data from " << experiment_dir << std::endl;
            return 1;
        }

        std::cout << "Data loaded successfully\n";
        std::cout << "Parsed graphs: " << coordinator.getDataManager().getNumGraphs() << "\n";
        std::cout << "Rules: " << coordinator.getRuleManager().getNumRules() << "\n";

        // Print algorithm configurations
        std::cout << "\n=== Algorithm Configurations ===\n";
        for (auto algo_type : config.algorithms_to_run) {
            std::string algo_name = EMAlgorithmFactory::algorithmTypeToString(algo_type);
            const auto& algo_config = config.algorithm_configs.at(algo_type);

            std::cout << algo_name << ":\n";
            std::cout << "  Max iterations: " << algo_config.max_iterations << "\n";
            std::cout << "  Convergence threshold: " << algo_config.convergence_threshold << "\n";

            if (algo_type == EMAlgorithmFactory::AlgorithmType::BATCH_EM) {
                std::cout << "  Batch size: " << algo_config.batch_size << "\n";
            }
            if (algo_type == EMAlgorithmFactory::AlgorithmType::ONLINE_EM) {
                std::cout << "  Learning rate: " << algo_config.learning_rate << "\n";
            }
            std::cout << "  Verbose: " << (algo_config.verbose ? "YES" : "NO") << "\n";
            if (algo_config.timeout_seconds > 0) {
                std::cout << "  Timeout: " << algo_config.timeout_seconds << "s\n";
            }
        }
        std::cout << "=================================\n";

        // Run complete experiment
        std::cout << "\nStarting multi-algorithm experiment...\n";
        EMExperimentResult result = coordinator.runCompleteExperiment();

        // Print comprehensive results
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "=== EXPERIMENT RESULTS SUMMARY ===\n";
        std::cout << std::string(60, '=') << "\n";

        std::cout << "Overall success: " << (result.overall_success ? "YES" : "NO") << "\n";
        std::cout << "Total experiment time: " << result.total_experiment_time.count() << "ms\n";

        if (!result.best_algorithm.empty()) {
            std::cout << "Best algorithm: " << result.best_algorithm
                     << " (score: " << result.best_score << ")\n";
        }

        std::cout << "\n--- Algorithm Results ---\n";
        for (const auto& [algo_name, algo_result] : result.algorithm_results) {
            std::cout << "\n" << algo_name << ":\n";
            std::cout << "  Success: " << (algo_result.successful ? "YES" : "NO") << "\n";
            if (algo_result.successful) {
                std::cout << "  Iterations: " << algo_result.iterations_completed << "\n";
                std::cout << "  Final LL: " << algo_result.final_log_likelihood << "\n";
                std::cout << "  Convergence: " << algo_result.convergence_value << "\n";
                std::cout << "  Time: " << algo_result.total_time.count() << "ms\n";
            }
        }

        if (config.run_evaluation && !result.evaluation_results.empty()) {
            std::cout << "\n--- Evaluation Results ---\n";
            for (const auto& [algo_name, eval_result] : result.evaluation_results) {
                std::cout << algo_name << ":\n";
                std::cout << "  Parsing accuracy: " << eval_result.parsing_accuracy.accuracy << "\n";
                std::cout << "  F1 score: " << eval_result.parsing_accuracy.f1_score << "\n";
                std::cout << "  Precision: " << eval_result.parsing_accuracy.precision << "\n";
                std::cout << "  Recall: " << eval_result.parsing_accuracy.recall << "\n";

                if (!eval_result.generation_metrics.empty()) {
                    std::cout << "  BLEU score: " << eval_result.generation_metrics[0].bleu_score << "\n";
                }
            }
        }

        // Performance comparison
        if (result.algorithm_results.size() > 1) {
            std::cout << "\n--- Performance Comparison ---\n";

            // Find fastest algorithm
            std::string fastest_algo;
            long min_time = std::numeric_limits<long>::max();
            for (const auto& [algo_name, algo_result] : result.algorithm_results) {
                if (algo_result.successful && algo_result.total_time.count() < min_time) {
                    min_time = algo_result.total_time.count();
                    fastest_algo = algo_name;
                }
            }

            // Find algorithm with best convergence
            std::string best_convergence_algo;
            double best_convergence = std::numeric_limits<double>::max();
            for (const auto& [algo_name, algo_result] : result.algorithm_results) {
                if (algo_result.successful && algo_result.convergence_value < best_convergence) {
                    best_convergence = algo_result.convergence_value;
                    best_convergence_algo = algo_name;
                }
            }

            if (!fastest_algo.empty()) {
                std::cout << "Fastest algorithm: " << fastest_algo << " (" << min_time << "ms)\n";
            }
            if (!best_convergence_algo.empty()) {
                std::cout << "Best convergence: " << best_convergence_algo << " (" << best_convergence << ")\n";
            }
        }

        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "Results saved to: " << config.output_directory << "\n";
        std::cout << std::string(60, '=') << "\n";

        return result.overall_success ? 0 : 1;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}