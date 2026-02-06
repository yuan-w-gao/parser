//
// Created by Yuan Gao on 22/09/2025.
// Run experiments for all folders in a directory
//

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <cstdlib>

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <base_directory> [algorithm]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  base_directory    Path to directory containing experiment folders" << std::endl;
    std::cout << "                   (e.g., /path/to/incremental/.../by_ages/)" << std::endl;
    std::cout << "  algorithm         Algorithm to run (default: all)" << std::endl;
    std::cout << "                   Options: batch_em, viterbi_em, online_em, full_em, all" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program_name << " /path/to/incremental/.../by_ages batch_em" << std::endl;
    std::cout << "  " << program_name << " /path/to/incremental/.../by_ages all" << std::endl;
}

std::vector<std::string> getExperimentFolders(const std::string& base_dir) {
    std::vector<std::string> folders;

    try {
        for (const auto& entry : std::filesystem::directory_iterator(base_dir)) {
            if (entry.is_directory()) {
                std::string folder_name = entry.path().filename().string();

                // Check if this looks like an experiment folder (numeric name)
                bool is_numeric = true;
                for (char c : folder_name) {
                    if (!std::isdigit(c)) {
                        is_numeric = false;
                        break;
                    }
                }

                if (is_numeric) {
                    // Check if required files exist
                    std::string grammar_file = entry.path() / (folder_name + ".mapping.txt");
                    std::string graph_file = entry.path() / (folder_name + ".graphs.txt");

                    if (std::filesystem::exists(grammar_file) &&
                        std::filesystem::exists(graph_file)) {
                        folders.push_back(entry.path().string());
                    }
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error accessing directory: " << e.what() << std::endl;
    }

    // Sort folders numerically
    std::sort(folders.begin(), folders.end(), [](const std::string& a, const std::string& b) {
        std::filesystem::path path_a(a);
        std::filesystem::path path_b(b);
        int num_a = std::stoi(path_a.filename().string());
        int num_b = std::stoi(path_b.filename().string());
        return num_a < num_b;
    });

    return folders;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string base_directory = argv[1];
    std::string algorithm = (argc >= 3) ? argv[2] : "all";

    // Validate base directory
    if (!std::filesystem::exists(base_directory) || !std::filesystem::is_directory(base_directory)) {
        std::cerr << "Error: Directory does not exist: " << base_directory << std::endl;
        return 1;
    }

    // Get experiment folders
    std::vector<std::string> experiment_folders = getExperimentFolders(base_directory);

    if (experiment_folders.empty()) {
        std::cerr << "Error: No valid experiment folders found in " << base_directory << std::endl;
        std::cerr << "Each folder should contain <name>.mapping.txt and <name>.graphs.txt files" << std::endl;
        return 1;
    }

    std::cout << "Found " << experiment_folders.size() << " experiment folders:" << std::endl;
    for (const auto& folder : experiment_folders) {
        std::filesystem::path path(folder);
        std::cout << "  " << path.filename().string() << std::endl;
    }
    std::cout << std::endl;

    // Run experiments
    int successful = 0;
    int failed = 0;

    for (const auto& experiment_dir : experiment_folders) {
        std::filesystem::path path(experiment_dir);
        std::string folder_name = path.filename().string();

        std::cout << "=== Running experiment for folder: " << folder_name << " ===" << std::endl;

        // Construct command
        std::string command = "./run_em_experiment " + algorithm + " " + experiment_dir;

        std::cout << "Command: " << command << std::endl;

        // Execute command
        int result = std::system(command.c_str());

        if (result == 0) {
            std::cout << "SUCCESS: Experiment " << folder_name << " completed" << std::endl;
            successful++;
        } else {
            std::cout << "FAILED: Experiment " << folder_name << " failed with code " << result << std::endl;
            failed++;
        }

        std::cout << std::endl;
    }

    // Summary
    std::cout << "=== SUMMARY ===" << std::endl;
    std::cout << "Total experiments: " << experiment_folders.size() << std::endl;
    std::cout << "Successful: " << successful << std::endl;
    std::cout << "Failed: " << failed << std::endl;

    return (failed == 0) ? 0 : 1;
}