//
// Test program for forest cache functionality
// Parses graphs, saves to cache, loads back, and verifies correctness
//

#include "manager.hpp"
#include "forest_cache.hpp"
#include "em_framework/em.hpp"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstring>

using namespace shrg;

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: test_forest_cache <parser_type> <grammar_path> <graph_path> [cache_dir]\n";
        std::cerr << "Example: test_forest_cache tree_v2 grammars/childes_cxg/06/train.mapping.txt grammars/childes_cxg/06/train.graphs.txt grammars/childes_cxg/06/cache\n";
        return 1;
    }

    std::string parser_type = argv[1];
    std::string grammar_path = argv[2];
    std::string graph_path = argv[3];
    std::string cache_dir = (argc > 4) ? argv[4] : "grammars/childes_cxg/06/cache";

    std::cout << "=== Forest Cache Test ===\n";
    std::cout << "Parser type: " << parser_type << "\n";
    std::cout << "Grammar: " << grammar_path << "\n";
    std::cout << "Graphs: " << graph_path << "\n";
    std::cout << "Cache dir: " << cache_dir << "\n\n";

    // Initialize manager
    auto* manager = &Manager::manager;
    manager->Allocate(1);
    manager->LoadGrammars(grammar_path);
    manager->LoadGraphs(graph_path);

    auto& context = manager->contexts[0];
    context->Init(parser_type, false, 100);

    std::vector<SHRG*> shrg_rules = manager->shrg_rules;

    std::cout << "Loaded " << shrg_rules.size() << " rules\n";
    std::cout << "Loaded " << manager->edsgraphs.size() << " graphs\n\n";

    // Create EM helper for parent pointer operations
    double threshold = 0.01 * manager->edsgraphs.size();
    em::EM em_helper(shrg_rules, manager->edsgraphs, context, threshold, "N", 5);

    // Create forest cache
    forest_cache::ForestCache cache(cache_dir);

    // Compute grammar hash
    std::string grammar_content;
    for (size_t ri = 0; ri < shrg_rules.size(); ri++) {
        const auto* rule = shrg_rules[ri];
        if (rule) {
            grammar_content += std::to_string(rule->label_hash) + ":";
            grammar_content += std::to_string(rule->terminal_edges.size()) + ";";
        }
    }
    uint32_t grammar_hash = forest_cache::ForestCache::compute_hash(grammar_content);
    cache.set_grammar_hash(grammar_hash);

    std::cout << "Grammar hash: 0x" << std::hex << grammar_hash << std::dec << "\n\n";

    // Persistent memory pool for cached forests
    utils::MemoryPool<ChartItem> persistent_pool;

    // Test all graphs
    const int num_test_graphs = static_cast<int>(manager->edsgraphs.size());

    // ========================================
    // Phase 1: Parse and save to cache
    // ========================================
    std::cout << "=== Phase 1: Parse and Cache ===\n";

    std::vector<int> parsed_graph_indices;
    std::vector<size_t> original_forest_sizes;
    parsed_graph_indices.reserve(num_test_graphs);
    original_forest_sizes.reserve(num_test_graphs);

    auto start_phase1 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_test_graphs; i++) {
        std::string graph_id = manager->edsgraphs[i].sentence_id;

        // Compute graph hash
        std::string graph_content = graph_id + ":" +
            std::to_string(manager->edsgraphs[i].nodes.size()) + ":" +
            std::to_string(manager->edsgraphs[i].edges.size());
        for (const auto& edge : manager->edsgraphs[i].edges) {
            graph_content += std::to_string(edge.label) + ",";
        }
        uint32_t graph_hash = forest_cache::ForestCache::compute_hash(graph_content);

        // Parse
        auto err = context->Parse(i);
        if (err != ParserError::kNone) {
            std::cout << "  Graph " << i << " (" << graph_id << "): parse failed\n";
            continue;
        }

        ChartItem* root = context->parser->Result();
        if (!root) {
            std::cout << "  Graph " << i << " (" << graph_id << "): no result\n";
            continue;
        }

        // Add parent pointers and rule pointers
        em_helper.addParentPointerOptimized(root, 0);
        em_helper.addRulePointer(root);

        // Count forest size
        size_t forest_size = em_helper.countForestSize(root);
        original_forest_sizes.push_back(forest_size);

        // Deep copy to persistent pool
        ChartItem* persistent_root = em_helper.deepCopyDerivationForest(root, persistent_pool);
        em_helper.addRulePointer(persistent_root);

        // Save to cache
        cache.save(graph_id, graph_hash, persistent_root);

        parsed_graph_indices.push_back(i);

        // Progress output every 50 graphs
        if ((i + 1) % 50 == 0 || i == num_test_graphs - 1) {
            std::cout << "  Processed " << (i + 1) << "/" << num_test_graphs << " graphs\n";
        }
    }

    auto end_phase1 = std::chrono::high_resolution_clock::now();
    double phase1_sec = std::chrono::duration<double>(end_phase1 - start_phase1).count();

    std::cout << "\nPhase 1 complete: " << parsed_graph_indices.size() << " graphs cached in "
              << std::fixed << std::setprecision(2) << phase1_sec << "s\n\n";

    // ========================================
    // Phase 2: Load from cache and verify
    // ========================================
    std::cout << "=== Phase 2: Load from Cache ===\n";

    utils::MemoryPool<ChartItem> load_pool;
    int cache_hits = 0;
    int cache_misses = 0;
    int verified = 0;

    auto start_phase2 = std::chrono::high_resolution_clock::now();

    for (size_t idx = 0; idx < parsed_graph_indices.size(); idx++) {
        int i = parsed_graph_indices[idx];
        std::string graph_id = manager->edsgraphs[i].sentence_id;

        // Compute same graph hash as before
        std::string graph_content = graph_id + ":" +
            std::to_string(manager->edsgraphs[i].nodes.size()) + ":" +
            std::to_string(manager->edsgraphs[i].edges.size());
        for (const auto& edge : manager->edsgraphs[i].edges) {
            graph_content += std::to_string(edge.label) + ",";
        }
        uint32_t graph_hash = forest_cache::ForestCache::compute_hash(graph_content);

        // Try to load from cache
        ChartItem* loaded_root = cache.load(graph_id, graph_hash, load_pool);

        if (loaded_root) {
            cache_hits++;

            // Restore rule pointers using the cached shrg_index
            forest_cache::ForestCache::restore_rule_pointers(loaded_root, shrg_rules);

            // Count forest size
            size_t loaded_size = em_helper.countForestSize(loaded_root);
            size_t original_size = original_forest_sizes[idx];

            bool size_match = (loaded_size == original_size);
            if (size_match) {
                verified++;
            } else {
                std::cout << "  Graph " << i << " (" << graph_id << "): SIZE MISMATCH "
                          << loaded_size << " vs " << original_size << "\n";
            }
        } else {
            cache_misses++;
            std::cout << "  Graph " << i << " (" << graph_id << "): CACHE MISS\n";
        }

        // Progress output every 50 graphs
        if ((idx + 1) % 50 == 0 || idx == parsed_graph_indices.size() - 1) {
            std::cout << "  Loaded " << (idx + 1) << "/" << parsed_graph_indices.size()
                      << " graphs (hits: " << cache_hits << ", verified: " << verified << ")\n";
        }
    }

    auto end_phase2 = std::chrono::high_resolution_clock::now();
    double phase2_sec = std::chrono::duration<double>(end_phase2 - start_phase2).count();

    std::cout << "\nPhase 2 complete: loaded " << cache_hits << " forests in "
              << std::fixed << std::setprecision(2) << phase2_sec << "s\n\n";

    // ========================================
    // Summary
    // ========================================
    std::cout << "=== Summary ===\n";
    std::cout << "Total graphs tested: " << num_test_graphs << "\n";
    std::cout << "Successfully parsed: " << parsed_graph_indices.size() << "\n";
    std::cout << "Cache hits: " << cache_hits << "\n";
    std::cout << "Cache misses: " << cache_misses << "\n";
    std::cout << "Verified correct: " << verified << "/" << cache_hits << "\n";
    std::cout << "\n";

    if (cache_hits > 0 && cache_misses == 0 && verified == cache_hits) {
        std::cout << "SUCCESS: Forest cache is working correctly!\n";
        return 0;
    } else {
        std::cout << "FAILURE: Forest cache has issues.\n";
        return 1;
    }
}
