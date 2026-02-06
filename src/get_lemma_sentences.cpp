//
// Created by Yuan Gao on 07/02/2025.
//

//
// Created by Yuan Gao on 09/12/2024.
//
//#include "em_framework/metrics.hpp"
#include "manager.hpp"

#include <fstream>

using namespace shrg;
const int VISITED = -2000;


void writeStringsToFile(const std::vector<std::string>& strings, const std::string& filename) {
    std::ofstream outFile(filename); // Open the file for writing

    if (!outFile) { // Check if the file was opened successfully
        std::cerr << "Error: Could not open the file " << filename << " for writing." << std::endl;
        return;
    }

    for (const auto& str : strings) { // Iterate over the vector of strings
        outFile << str << std::endl; // Write each string followed by a newline
    }

    outFile.close(); // Close the file
}

int main(int argc, char* argv[]) {
    auto *manager = &Manager::manager;
    manager->Allocate(1);
    if (argc < 2){
        std::cout << "wrrrrrong";
        return 1;
    }

    manager->LoadGrammars(argv[2]);
    manager->LoadGraphs(argv[3]);
    auto &context = manager->contexts[0];
    context->Init(argv[1], false, 100);
    std::string outDir = argv[4];

    std::vector<std::string> unlemma;

    for (auto graph: manager->edsgraphs) {
        auto code = context->parser->Parse(graph);
        if(code == ParserError::kNone) {
            unlemma.push_back(graph.lemma_sequence);
        }
    }



    // std::cout << "num equals: " << num_equals << std::endl;

    writeStringsToFile(unlemma, outDir+"reference_sentences.txt");

    return 0;
}
