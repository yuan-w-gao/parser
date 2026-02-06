//
// Created by Yuan Gao on 22/09/2025.
// Single experiment configuration for EM algorithms
//

#pragma once

#include <string>
#include <filesystem>

namespace shrg {
namespace experiment {

struct ExperimentConfig {
    // Input paths (required)
    std::string experiment_dir;  // e.g., "/path/to/incremental/.../by_ages/10/"

    // Algorithm parameters
    std::string parser_type = "tree_v2";  // parser type
    double threshold = 0.01;               // convergence threshold (as fraction of graph count)
    int max_iterations = 50;               // maximum EM iterations
    int batch_size = 5;                    // for batch EM
    int timeout_seconds = 300;             // timeout for experiments

    // Evaluation settings
    bool run_evaluation = true;            // whether to run evaluation
    bool save_derivations = true;          // whether to save derivation files

    // Constructor
    ExperimentConfig() = default;
    ExperimentConfig(const std::string& dir) : experiment_dir(dir) {}

    // Auto-detect input files based on experiment_dir
    std::string getGrammarFile() const {
        std::filesystem::path dir_path(experiment_dir);
        std::string folder_name = dir_path.filename().string();
        return experiment_dir + "/" + folder_name + ".mapping.txt";
    }

    std::string getGraphFile() const {
        std::filesystem::path dir_path(experiment_dir);
        std::string folder_name = dir_path.filename().string();
        return experiment_dir + "/" + folder_name + ".graphs.txt";
    }

    // Output directory (create "induced_outputs" subdirectory)
    std::string getOutputDir() const {
        return experiment_dir + "/induced_outputs";
    }

    // Generate algorithm-specific output file paths
    std::string getDerivationOutputFile(const std::string& algorithm_name) const {
        return getOutputDir() + "/" + algorithm_name + "_derivations.txt";
    }

    std::string getEvaluationOutputFile(const std::string& algorithm_name) const {
        return getOutputDir() + "/" + algorithm_name + "_evaluation.json";
    }

    std::string getLogFile(const std::string& algorithm_name) const {
        return getOutputDir() + "/" + algorithm_name + "_log.txt";
    }

    // Validation
    bool isValid() const {
        namespace fs = std::filesystem;
        return !experiment_dir.empty() &&
               fs::exists(getGrammarFile()) &&
               fs::exists(getGraphFile());
    }

    // Get experiment name from directory
    std::string getExperimentName() const {
        std::filesystem::path dir_path(experiment_dir);
        return dir_path.filename().string();
    }
};

enum class AlgorithmType {
    BATCH_EM,
    VITERBI_EM,
    ONLINE_EM,
    FULL_EM
};

std::string algorithmTypeToString(AlgorithmType type);
AlgorithmType stringToAlgorithmType(const std::string& str);

} // namespace experiment
} // namespace shrg