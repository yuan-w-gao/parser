//
// Created by Yuan Gao on 05/06/2024.
//
#include <fstream>
#include "em_debug.hpp"

namespace shrg{
namespace em_debug{
const int VISITED_FLAG = 1;
void ForestInfo::TraverseForest(ChartItem* root_ptr, ForestInfo& info, Generator *generator, int depth) {
    if (!root_ptr || root_ptr->status == VISITED_FLAG)
        return;

    ChartItem* ptr = root_ptr;
    do {
        info.num_nodes++;
        info.width_at_depth[depth]++;
        if (depth > info.max_depth)
            info.max_depth = depth;

        ptr->status = VISITED_FLAG;

        const SHRG* grammar_ptr = ptr->attrs_ptr->grammar_ptr;
        for (auto edge_ptr : grammar_ptr->nonterminal_edges) {
            ChartItem* child = generator->FindChartItemByEdge(ptr, edge_ptr);
            TraverseForest(child, info, generator, depth + 1);
        }

        ptr = ptr->next_ptr;
    } while (ptr != root_ptr);

    // Reset status to allow re-traversal if needed
    ptr = root_ptr;
    do {
        ptr->status = 0;
        ptr = ptr->next_ptr;
    } while (ptr != root_ptr);
}


ForestInfo ForestInfo::GetForestInfo(Generator* generator, ChartItem* root_ptr) {
    ForestInfo info;
    TraverseForest(root_ptr, info, generator);
    return info;
}

void ForestInfo::PrintForestInfo(const ForestInfo& info) {
    std::cout << "Number of nodes: " << info.num_nodes << std::endl;
    std::cout << "Max depth: " << info.max_depth << std::endl;
    std::cout << "Width at each depth:" << std::endl;
    for (const auto& [depth, width] : info.width_at_depth) {
        std::cout << "Depth " << depth << ": " << width << " nodes" << std::endl;
    }
}

ChartItem* deepCopyChartItem(ChartItem* root) {
    if (!root) return nullptr;

    ChartItem* newRoot = new ChartItem(*root);
    std::unordered_map<ChartItem*, ChartItem*> oldToNew;
    oldToNew[root] = newRoot;

    std::queue<ChartItem*> queue;
    queue.push(root);

    while (!queue.empty()) {
        ChartItem* current = queue.front();
        queue.pop();

        for (auto child : current->children) {
            if (oldToNew.find(child) == oldToNew.end()) {
                oldToNew[child] = new ChartItem(*child);
                queue.push(child);
            }
            oldToNew[current]->children.push_back(oldToNew[child]);
        }

        if (current->next_ptr && oldToNew.find(current->next_ptr) == oldToNew.end()) {
            oldToNew[current->next_ptr] = new ChartItem(*current->next_ptr);
            queue.push(current->next_ptr);
        }
        oldToNew[current]->next_ptr = oldToNew[current->next_ptr];
    }

    return newRoot;
}

bool compareChartItems(ChartItem* root1, ChartItem* root2) {
    if (!root1 && !root2) return true;
    if (!root1 || !root2) return false;

    std::queue<ChartItem*> queue1, queue2;
    queue1.push(root1);
    queue2.push(root2);

    while (!queue1.empty() && !queue2.empty()) {
        ChartItem* node1 = queue1.front();
        ChartItem* node2 = queue2.front();
        queue1.pop();
        queue2.pop();

        // Skip if already visited
        if (node1->inside_visited_status == 1 && node2->inside_visited_status == 1) continue;

        // Mark as visited
        node1->inside_visited_status = 1;
        node2->inside_visited_status = 1;

        if (node1->level != node2->level) {
            return false;
        }
        if (node1->child_visited_status != node2->child_visited_status){
            return false;
        }
        if (node1->children.size() != node2->children.size()){
            return false;
        }
        if (node1->parents_sib.size() != node2->parents_sib.size()){
            return false;
        }

        for (size_t i = 0; i < node1->children.size(); ++i) {
            queue1.push(node1->children[i]);
            queue2.push(node2->children[i]);
        }

        for (size_t i = 0; i < node1->parents_sib.size(); ++i) {
            if (std::get<0>(node1->parents_sib[i]) != std::get<0>(node2->parents_sib[i])) {
                return false;
            }
            if (std::get<1>(node1->parents_sib[i]).size() != std::get<1>(node2->parents_sib[i]).size()) {
                return false;
            }
            for (size_t j = 0; j < std::get<1>(node1->parents_sib[i]).size(); ++j) {
                if (std::get<1>(node1->parents_sib[i])[j] != std::get<1>(node2->parents_sib[i])[j]) {
                    return false;
                }
            }
        }

        if (node1->next_ptr || node2->next_ptr) {
            if (!node1->next_ptr || !node2->next_ptr){
                return false;
            }
            queue1.push(node1->next_ptr);
            queue2.push(node2->next_ptr);
        }
    }

    return queue1.empty() && queue2.empty();
}

void deleteChartItem(ChartItem* root) {
    if (!root) return;

    std::unordered_map<ChartItem*, bool> visited;
    std::queue<ChartItem*> queue;
    queue.push(root);

    while (!queue.empty()) {
        ChartItem* current = queue.front();
        queue.pop();

        if (visited[current]) continue;
        visited[current] = true;

        for (auto child : current->children) {
            if (!visited[child]) {
                queue.push(child);
            }
        }

        if (current->next_ptr && !visited[current->next_ptr]) {
            queue.push(current->next_ptr);
        }

        delete current;
    }
}


}
}