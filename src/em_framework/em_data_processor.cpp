#include "em_data_processor.hpp"
#include <queue>

namespace shrg::em{
const int EM_DATA_PROCESSOR::VISITED = -2000;
EM_DATA_PROCESSOR:: EM_DATA_PROCESSOR(std::vector<EdsGraph> &graphs, shrg::Context *context)
    : context(context), graphs(graphs) {
}

void EM_DATA_PROCESSOR::parseAllGraphs(){
    for(int i = 0; i < graphs.size(); i++){
        EdsGraph graph = graphs[i];
        auto code = context->Parse(graph);
        if(code == ParserError::kNone){
            ChartItem *root = context->parser->Result();
            forests.push_back(root);
        }else{
            std::cout << i << " couldn't parse\n";
        }
    }
}

ItemList& EM_DATA_PROCESSOR::getForests(){
    return forests;
}



void EM_DATA_PROCESSOR::addParentPointer(int index){
    if(index < 0 || index >= forests.size()){
        std::cout << "Invalid index " << index << "\n";
        return;
    }

}

void addParentPointer_all(){

}

//===================================non public functions=================
void EM_DATA_PROCESSOR::addParentPointer(ChartItem *root, int level){
    ChartItem *ptr = root;
    Generator *generator = context->parser->GetGenerator();

    do {
        if (level > ptr->level) {
            ptr->level = level;
        }

        const SHRG *rule = ptr->attrs_ptr->grammar_ptr;
        if (ptr->child_visited_status != EM_DATA_PROCESSOR::VISITED) {
            for (auto edge_ptr : rule->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                addParentPointer(child, ptr->level + 1);
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
            ptr->child_visited_status = EM_DATA_PROCESSOR::VISITED;
        } else {
            for (auto child : ptr->children) {
                addParentPointer(child, ptr->level + 1);
            }
        }
        assert(ptr->children.size() == rule->nonterminal_edges.size());

        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void EM_DATA_PROCESSOR::addParentPointerOptimized(ChartItem *root, int level) {
    if (!root) return;

    // Use a queue to implement BFS-like traversal
    std::queue<std::pair<ChartItem*, int>> queue;
    queue.push({root, level});

    Generator *generator = context->parser->GetGenerator();

    while (!queue.empty()) {
        auto [ptr, level] = queue.front();
        queue.pop();

        if (level > ptr->level) {
            ptr->level = level;
        }

        const SHRG *rule = ptr->attrs_ptr->grammar_ptr;

        if (ptr->child_visited_status != EM_DATA_PROCESSOR::VISITED) {
            ptr->children.reserve(rule->nonterminal_edges.size());

            // Find and process children
            for (auto edge_ptr : rule->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                queue.push({child, ptr->level + 1});
            }

            // Precompute siblings once and reuse
            size_t childCount = ptr->children.size();
            std::vector<std::vector<ChartItem*>> siblings(childCount);

            for (size_t i = 0; i < childCount; ++i) {
                for (size_t j = 0; j < childCount; ++j) {
                    if (i != j) {
                        siblings[i].push_back(ptr->children[j]);
                    }
                }
            }

            // Assign parent and sibling information
            for (size_t i = 0; i < childCount; ++i) {
                ptr->children[i]->parents_sib.push_back({ptr, std::move(siblings[i])});
            }

            ptr->child_visited_status = EM_DATA_PROCESSOR::VISITED;
        } else {
            // Process children of already visited nodes to ensure full traversal
            for (auto child : ptr->children) {
                queue.push({child, ptr->level + 1});
            }
        }

        assert(ptr->children.size() == rule->nonterminal_edges.size());
    }
}



}