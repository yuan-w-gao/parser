//
// Created by Yuan Gao on 22/09/2025.
// Modular experiment runner for single EM experiments
//

#pragma once

#include "experiment_config.hpp"
#include "../manager.hpp"
#include "../em_framework/em_base.hpp"
#include "../em_framework/em_batch.hpp"
#include "../em_framework/em_viterbi.hpp"
#include "../em_framework/em_online.hpp"
#include "../em_framework/em.hpp"
#include "../em_framework/em_evaluate/em_evaluate.hpp"
#include <memory>
#include <chrono>

namespace shrg {
namespace experiment {

class ExperimentRunner {
public:
    explicit ExperimentRunner(const ExperimentConfig& config);
    ~ExperimentRunner();

    // Main experiment execution
    bool runExperiment(AlgorithmType algorithm);
    bool runAllAlgorithms();

    // Individual steps (can be called separately for debugging)
    bool setupExperiment();
    bool loadData();
    std::unique_ptr<em::EMBase> createAlgorithm(AlgorithmType algorithm);
    bool runEM(em::EMBase* em_algorithm, const std::string& algorithm_name);
    bool runEvaluation(em::EMBase* em_algorithm, const std::string& algorithm_name);
    bool saveResults(em::EMBase* em_algorithm, const std::string& algorithm_name);

    // Utility methods
    void setVerbose(bool verbose) { verbose_ = verbose; }
    const ExperimentConfig& getConfig() const { return config_; }

private:
    ExperimentConfig config_;
    bool verbose_ = true;
    bool setup_complete_ = false;

    // Manager and context (reused across algorithms)
    Manager* manager_;
    Context* context_;

    // Data (loaded once, reused)
    std::vector<EdsGraph> graphs_;
    std::vector<SHRG*> shrg_rules_;

    // Logging and timing
    std::chrono::steady_clock::time_point start_time_;

    // Helper methods
    bool createOutputDirectory();
    void logMessage(const std::string& message) const;
    void logError(const std::string& error) const;
    double calculateThreshold() const;
    bool validateInputFiles() const;

    // EM algorithm factories
    std::unique_ptr<em::BatchEM> createBatchEM();
    std::unique_ptr<em::ViterbiEM> createViterbiEM();
    std::unique_ptr<em::OnlineEM> createOnlineEM();
    std::unique_ptr<em::EM> createFullEM();
};

} // namespace experiment
} // namespace shrg