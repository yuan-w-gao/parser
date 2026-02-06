//
// Created by Yuan Gao on 05/06/2024.
//
#include "manager.hpp"

//#include "python/inside_outside.cpp"
#include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"
#include "em_framework/em.hpp"
#include "em_framework/em_debug.hpp"


#include <set>
#include <iostream>
#include <map>

using namespace shrg;
using namespace em_debug;

const int VISITED = -1000;

void addParentPointer(Context *context, ChartItem *root, int level){
    ChartItem *ptr = root;
    Generator *generator = context->parser->GetGenerator();

    do {
        if (level > ptr->level) {
            ptr->level = level;
        }

        const SHRG *rule = ptr->attrs_ptr->grammar_ptr;
        if (ptr->child_visited_status != VISITED) {
            for (auto edge_ptr : rule->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                addParentPointer(context, child, ptr->level + 1);
            }
            for (int i = 0; i < ptr->children.size(); i++) {
                std::vector<ChartItem *> sib;
                std::tuple<ChartItem *, std::vector<ChartItem *>> res;
                for (int j = 0; j < ptr->children.size(); j++) {
                    if (j == i) {
                        continue;
                    }
                    sib.push_back(ptr->children[j]);
                }

                if (ptr) {
                    res = std::make_tuple(ptr, sib);
                    ptr->children[i]->parents_sib.push_back(res);
                }
            }
            ptr->child_visited_status = VISITED;
        } else {
            for (auto child : ptr->children) {
                addParentPointer(context, child, ptr->level + 1);
            }
        }
        assert(ptr->children.size() == rule->nonterminal_edges.size());

        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void addParentPointerOptimized(Context *context, ChartItem *root, int level) {
    if (!root) return;

    // Use a queue to implement BFS-like traversal
    std::queue<std::pair<ChartItem*, int>> queue;
    queue.push({root, level});

    Generator *generator = context->parser->GetGenerator();

    while (!queue.empty()) {
        auto [ptr, level] = queue.front();
        queue.pop();

        ChartItem *current = ptr;
        do {
            if (level > current->level) {
                current->level = level;
            }

            const SHRG *rule = current->attrs_ptr->grammar_ptr;

            if (current->child_visited_status != VISITED) {
                current->children.reserve(rule->nonterminal_edges.size());

                // Find and process children
                for (auto edge_ptr : rule->nonterminal_edges) {
                    ChartItem *child = generator->FindChartItemByEdge(current, edge_ptr);
                    current->children.push_back(child);
                    queue.push({child, current->level + 1});
                }

                // Precompute siblings once and reuse
                size_t childCount = current->children.size();
                std::vector<std::vector<ChartItem*>> siblings(childCount);

                for (size_t i = 0; i < childCount; ++i) {
                    for (size_t j = 0; j < childCount; ++j) {
                        if (i != j) {
                            siblings[i].push_back(current->children[j]);
                        }
                    }
                }

                // Assign parent and sibling information
                for (size_t i = 0; i < childCount; ++i) {
                    current->children[i]->parents_sib.push_back({current, std::move(siblings[i])});
                }

                current->child_visited_status = VISITED;
            } else {
                // Process children of already visited nodes to ensure full traversal
                for (auto child : current->children) {
                    queue.push({child, current->level + 1});
                }
            }

            assert(current->children.size() == rule->nonterminal_edges.size());

            current = current->next_ptr;
        } while (current != ptr);
    }
}


void check_parent(Context *context, ChartItem *root){
    ChartItem *root1 = deepCopyChartItem(root);
    ChartItem *root2 = deepCopyChartItem(root);

    addParentPointer(context, root1, 0);
    addParentPointerOptimized(context, root2, 0);

    if (compareChartItems(root1, root2)) {
        std::cout << "Both versions produce the same result." << std::endl;
    } else {
        std::cout << "The versions produce different results." << std::endl;
    }

    // Cleanup
    deleteChartItem(root1);
    deleteChartItem(root2);
}

int main(int argc, char *argv[]) {
    auto *manager = &Manager::manager;
    manager->Allocate(1);
    if (argc != 4) {
        LOG_ERROR("Usage: generate <parser_type> <grammar_path> <graph_path>");
        return 1;
    }

    manager->LoadGrammars(argv[2]);
    manager->LoadGraphs(argv[3]);
    auto &context = manager->contexts[0];
    context->Init(argv[1], false , 100 );


    auto graphs = manager->edsgraphs;
    std::vector<SHRG *>shrg_rules = manager->shrg_rules;
//    shrg::em::EM model = shrg::em::EM(shrg_rules, graphs, context, -0.03);
//    model.run();

    auto graph = graphs[3];
    auto code = context->Parse(graph);
    if(code == ParserError::kNone) {
        ChartItem *root = context->parser->Result();
        check_parent(context, root);
    }

    return 0;
}