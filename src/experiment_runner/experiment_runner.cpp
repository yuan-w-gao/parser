//
// Created by Yuan Gao on 22/09/2025.
//

#include "experiment_runner.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace shrg {
namespace experiment {

ExperimentRunner::ExperimentRunner(const ExperimentConfig& config)
    : config_(config), manager_(&Manager::manager), context_(nullptr) {
    start_time_ = std::chrono::steady_clock::now();
}

ExperimentRunner::~ExperimentRunner() {
    // Cleanup is handled by Manager singleton
}

bool ExperimentRunner::runExperiment(AlgorithmType algorithm) {
    std::string algorithm_name = algorithmTypeToString(algorithm);
    logMessage("Starting experiment: " + config_.getExperimentName() + " with " + algorithm_name);

    if (!setupExperiment()) {
        logError("Failed to setup experiment");
        return false;
    }

    auto em_algorithm = createAlgorithm(algorithm);
    if (!em_algorithm) {
        logError("Failed to create " + algorithm_name + " algorithm");
        return false;
    }

    if (!runEM(em_algorithm.get(), algorithm_name)) {
        logError("Failed to run " + algorithm_name);
        return false;
    }

    if (config_.run_evaluation) {
        if (!runEvaluation(em_algorithm.get(), algorithm_name)) {
            logError("Failed to run evaluation for " + algorithm_name);
            return false;
        }
    }

    if (!saveResults(em_algorithm.get(), algorithm_name)) {
        logError("Failed to save results for " + algorithm_name);
        return false;
    }

    logMessage("Completed experiment: " + algorithm_name);
    return true;
}

bool ExperimentRunner::runAllAlgorithms() {
    std::vector<AlgorithmType> algorithms = {
        AlgorithmType::BATCH_EM,
        AlgorithmType::VITERBI_EM,
        AlgorithmType::ONLINE_EM,
        AlgorithmType::FULL_EM
    };

    bool all_success = true;
    for (auto algorithm : algorithms) {
        if (!runExperiment(algorithm)) {
            all_success = false;
            logError("Failed experiment: " + algorithmTypeToString(algorithm));
        }
    }

    return all_success;
}

bool ExperimentRunner::setupExperiment() {
    if (setup_complete_) return true;

    logMessage("Setting up experiment for directory: " + config_.experiment_dir);

    if (!config_.isValid()) {
        logError("Invalid experiment configuration");
        return false;
    }

    if (!createOutputDirectory()) {
        return false;
    }

    if (!loadData()) {
        return false;
    }

    setup_complete_ = true;
    return true;
}

bool ExperimentRunner::loadData() {
    logMessage("Loading data...");

    // Allocate manager
    manager_->Allocate(1);

    // Load grammars and graphs
    std::string grammar_file = config_.getGrammarFile();
    std::string graph_file = config_.getGraphFile();

    logMessage("Loading grammar: " + grammar_file);
    manager_->LoadGrammars(grammar_file);

    logMessage("Loading graphs: " + graph_file);
    manager_->LoadGraphs(graph_file);

    // Initialize context
    context_ = manager_->contexts[0];
    context_->Init(config_.parser_type, false, 100);

    // Store references
    graphs_ = manager_->edsgraphs;
    shrg_rules_ = manager_->shrg_rules;

    logMessage("Loaded " + std::to_string(shrg_rules_.size()) + " rules and " +
               std::to_string(graphs_.size()) + " graphs");

    return !graphs_.empty() && !shrg_rules_.empty();
}

std::unique_ptr<em::EMBase> ExperimentRunner::createAlgorithm(AlgorithmType algorithm) {
    switch (algorithm) {
        case AlgorithmType::BATCH_EM:
            return createBatchEM();
        case AlgorithmType::VITERBI_EM:
            return createViterbiEM();
        case AlgorithmType::ONLINE_EM:
            return createOnlineEM();
        case AlgorithmType::FULL_EM:
            return createFullEM();
        default:
            logError("Unknown algorithm type");
            return nullptr;
    }
}

std::unique_ptr<em::BatchEM> ExperimentRunner::createBatchEM() {
    double threshold = calculateThreshold();
    std::string output_dir = config_.getOutputDir();

    return std::make_unique<em::BatchEM>(
        shrg_rules_, graphs_, context_, threshold,
        config_.batch_size, output_dir, config_.timeout_seconds
    );
}

std::unique_ptr<em::ViterbiEM> ExperimentRunner::createViterbiEM() {
    double threshold = calculateThreshold();
    std::string output_dir = config_.getOutputDir();

    return std::make_unique<em::ViterbiEM>(
        shrg_rules_, graphs_, context_, threshold,
        output_dir, config_.timeout_seconds
    );
}

std::unique_ptr<em::OnlineEM> ExperimentRunner::createOnlineEM() {
    double threshold = calculateThreshold();
    std::string output_dir = config_.getOutputDir();

    return std::make_unique<em::OnlineEM>(
        shrg_rules_, graphs_, context_, threshold,
        output_dir, config_.timeout_seconds
    );
}

std::unique_ptr<em::EM> ExperimentRunner::createFullEM() {
    double threshold = calculateThreshold();

    return std::make_unique<em::EM>(
        shrg_rules_, graphs_, context_, threshold
    );
}

bool ExperimentRunner::runEM(em::EMBase* em_algorithm, const std::string& algorithm_name) {
    logMessage("Running " + algorithm_name + "...");

    auto start = std::chrono::steady_clock::now();
    em_algorithm->run();
    auto end = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    logMessage("Completed " + algorithm_name + " in " + std::to_string(duration.count()) + " seconds");

    return true;
}

bool ExperimentRunner::runEvaluation(em::EMBase* em_algorithm, const std::string& algorithm_name) {
    logMessage("Running evaluation for " + algorithm_name + "...");

    try {
        em::EM_EVALUATE evaluator(em_algorithm);
        auto metrics = evaluator.evaluateAll();

        // Save evaluation results to JSON
        std::string eval_file = config_.getEvaluationOutputFile(algorithm_name);
        std::ofstream out(eval_file);
        if (out.is_open()) {
            out << "{\n";
            out << "  \"algorithm\": \"" << algorithm_name << "\",\n";
            out << "  \"bleu\": " << metrics.bleu << ",\n";
            out << "  \"f1\": " << metrics.f1 << ",\n";
            out << "  \"greedy_tree_score\": " << metrics.greedy_tree_score << ",\n";
            out << "  \"global_tree_score\": " << metrics.global_tree_score << "\n";
            out << "}\n";
            out.close();

            logMessage("Saved evaluation results to: " + eval_file);
            logMessage("BLEU: " + std::to_string(metrics.bleu) +
                      ", F1: " + std::to_string(metrics.f1));
        }

        return true;
    } catch (const std::exception& e) {
        logError("Evaluation failed: " + std::string(e.what()));
        return false;
    }
}

bool ExperimentRunner::saveResults(em::EMBase* em_algorithm, const std::string& algorithm_name) {
    if (!config_.save_derivations) {
        return true;
    }

    logMessage("Saving derivation results for " + algorithm_name + "...");

    // The EM algorithms should already save derivations to their output directories
    // This method can be extended to save additional results if needed

    return true;
}

bool ExperimentRunner::createOutputDirectory() {
    std::string output_dir = config_.getOutputDir();

    try {
        std::filesystem::create_directories(output_dir);
        logMessage("Created output directory: " + output_dir);
        return true;
    } catch (const std::filesystem::filesystem_error& e) {
        logError("Failed to create output directory: " + std::string(e.what()));
        return false;
    }
}

void ExperimentRunner::logMessage(const std::string& message) const {
    if (verbose_) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
        std::cout << "[" << std::setw(6) << elapsed.count() << "s] " << message << std::endl;
    }
}

void ExperimentRunner::logError(const std::string& error) const {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time_);
    std::cerr << "[" << std::setw(6) << elapsed.count() << "s] ERROR: " << error << std::endl;
}

double ExperimentRunner::calculateThreshold() const {
    return config_.threshold * graphs_.size();
}

bool ExperimentRunner::validateInputFiles() const {
    namespace fs = std::filesystem;

    std::string grammar_file = config_.getGrammarFile();
    std::string graph_file = config_.getGraphFile();

    if (!fs::exists(grammar_file)) {
        logError("Grammar file not found: " + grammar_file);
        return false;
    }

    if (!fs::exists(graph_file)) {
        logError("Graph file not found: " + graph_file);
        return false;
    }

    return true;
}

} // namespace experiment
} // namespace shrg