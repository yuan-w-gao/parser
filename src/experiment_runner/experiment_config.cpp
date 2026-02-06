//
// Created by Yuan Gao on 22/09/2025.
//

#include "experiment_config.hpp"
#include <stdexcept>

namespace shrg {
namespace experiment {

std::string algorithmTypeToString(AlgorithmType type) {
    switch (type) {
        case AlgorithmType::BATCH_EM:
            return "batch_em";
        case AlgorithmType::VITERBI_EM:
            return "viterbi_em";
        case AlgorithmType::ONLINE_EM:
            return "online_em";
        case AlgorithmType::FULL_EM:
            return "full_em";
        default:
            throw std::invalid_argument("Unknown algorithm type");
    }
}

AlgorithmType stringToAlgorithmType(const std::string& str) {
    if (str == "batch_em") return AlgorithmType::BATCH_EM;
    if (str == "viterbi_em") return AlgorithmType::VITERBI_EM;
    if (str == "online_em") return AlgorithmType::ONLINE_EM;
    if (str == "full_em") return AlgorithmType::FULL_EM;
    throw std::invalid_argument("Unknown algorithm string: " + str);
}

} // namespace experiment
} // namespace shrg