//
// Created by Yuan Gao on 5/24/23.
//

#include "manager.hpp"

#include "graph_parser/parser_tree_base.hpp"

#include "graph_parser/parser_linear.hpp"

#include "graph_parser/generator.hpp"

// #include "python/find_best_derivation.cpp"

// #include "python/inside_outside.cpp"
#include "graph_parser/synchronous_hyperedge_replacement_grammar.hpp"

#include <set>
#include <iostream>
#include <map>
#include <chrono>

using namespace shrg;


void traverse_label(ChartItem *root_ptr, Generator *generator,
                    std::map<Label, int> &edge_count,
                    std::map<Label, int> &grammars_count,
                    std::set<Label> &edges,
                    std::set<Label> &grammars){
    if (root_ptr->status == VISITED_FLAG)
        return ;
    auto ptr = root_ptr;
    do {
        const SHRG *grammar_ptr = ptr->attrs_ptr->grammar_ptr;
        Label grammar_label = grammar_ptr->label;
        grammars.insert(grammar_label);
        grammars_count[grammar_label] += 1;
        for (auto edge_ptr : grammar_ptr->nonterminal_edges){
            Label edge_label = edge_ptr->label;
            edges.insert(edge_label);
            edge_count[edge_label] += 1;
            traverse_label(generator->FindChartItemByEdge(ptr, edge_ptr), generator, edge_count, grammars_count, edges, grammars);
        }

        ptr = ptr->next_ptr;
    } while (ptr != root_ptr);

    root_ptr->status = VISITED_FLAG;
}

void checkChildrenSizeAndRulePtr(ChartItem *root, std::vector<SHRG *> &rules){
    ChartItem *ptr = root;

    do{
        int non_terminal_size = ptr->attrs_ptr->grammar_ptr->nonterminal_edges.size();
        assert(non_terminal_size == ptr->children.size());

        int shrg_index = ptr->attrs_ptr->grammar_ptr->best_cfg_ptr->shrg_index;
        assert(ptr->rule_ptr == rules[shrg_index]);

        for(auto child:ptr->children){
            checkChildrenSizeAndRulePtr(child, rules);
        }

        ptr = ptr->next_ptr;
    }while(ptr != root);
}

bool hasDup(std::vector<ChartItem*> &parents){
    std::set<ChartItem*> parent_set(parents.begin(), parents.end());
    return parent_set.size() != parents.size();
}

//const int VISITED_FLAG = 1;
struct TreeInfo {
    int num_nodes;
    int max_depth;
    std::unordered_map<int, int> width_at_depth;

    TreeInfo() : num_nodes(0), max_depth(0) {}
};

void TraverseTree(ChartItem* root_ptr, TreeInfo& info, Generator *generator, int depth = 0) {
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
            TraverseTree(child, info, generator, depth + 1);
        }

        ptr = ptr->next_ptr;
    } while (ptr != root_ptr);

    // Reset status to allow re-traversal if needed
    //    ptr = root_ptr;
    //    do {
    //        ptr->status = 0;
    //        ptr = ptr->next_ptr;
    //    } while (ptr != root_ptr);
}

TreeInfo GetForestInfo(Generator* generator, ChartItem* root_ptr) {
    TreeInfo info;
    TraverseTree(root_ptr, info, generator);
    return info;
}

void PrintForestInfo(const TreeInfo& info) {
    std::cout << "Number of nodes: " << info.num_nodes << std::endl;
    std::cout << "Max depth: " << info.max_depth << std::endl;
    std::cout << "Width at each depth:" << std::endl;
    for (const auto& [depth, width] : info.width_at_depth) {
        std::cout << "Depth " << depth << ": " << width << " nodes" << std::endl;
    }
}


void printBT(const std::string& prefix, const ChartItem *root, bool isLeft)
{
    if( root != nullptr )
    {
        std::cout << prefix;

        std::cout << (isLeft ? "├──" : "└──" );

        // print the value of the node
        std::cout << root->shrg_index << std::endl;

        // enter the next tree level - left and right branch
        std::vector<ChartItem*> children = root->children;
        if(children.size() == 0){
            return;
        }
        assert(children.size() == 2);
        printBT( prefix + (isLeft ? "│   " : "    "), children[0], true);
        printBT( prefix + (isLeft ? "│   " : "    "), children[1], false);
    }
}

void printBT(const ChartItem* node)
{
    printBT("", node, false);
}

void addParentPointer(ChartItem *root, int level, Generator *generator){
    //    std::cout << "addParentPointer" << std::endl;
    ChartItem *ptr = root;

    do {
        if (level > ptr->level) {
            ptr->level = level;
        }

        const SHRG *rule = ptr->attrs_ptr->grammar_ptr;
        if (ptr->child_visited_status !=VISITED_FLAG) {
            for (auto edge_ptr : rule->nonterminal_edges) {
                ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                ptr->children.push_back(child);
                addParentPointer(child, ptr->level + 1, generator);
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
            ptr->child_visited_status = VISITED_FLAG;
        } else {
            for (auto child : ptr->children) {
                addParentPointer(child, ptr->level + 1, generator);
            }
        }
        assert(ptr->children.size() == rule->nonterminal_edges.size());

        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void addParentPointerOptimized(ChartItem *root, int level, Generator *generator) {
//        std::cout << "addParentPointerOptimized" << std::endl;
    if (!root) return;

    std::queue<std::pair<ChartItem*, int>> queue;
    queue.push({root, level});

    while (!queue.empty()) {
        auto [ptr1, level] = queue.front();
        queue.pop();
        auto ptr = ptr1;
        do{
                if (level > ptr->level) {
                    ptr->level = level;
                }

                const SHRG *rule = ptr->attrs_ptr->grammar_ptr;

                if (ptr->child_visited_status != VISITED_FLAG) {
                    ptr->children.reserve(rule->nonterminal_edges.size());

                    for (auto edge_ptr : rule->nonterminal_edges) {
                        ChartItem *child = generator->FindChartItemByEdge(ptr, edge_ptr);
                        ptr->children.push_back(child);
                        queue.push({child, ptr->level + 1});
                    }

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

                    ptr->child_visited_status = VISITED_FLAG;
                } else {
                    for (auto child : ptr->children) {
                        queue.push({child, ptr->level + 1});
                    }
                }

                assert(ptr->children.size() == rule->nonterminal_edges.size());
                ptr = ptr->next_ptr;
        }while(ptr1 != ptr);
    }
}

void addRulePointer_test(ChartItem *root, std::vector<SHRG*> &shrg_rules) {
    if (root->rule_visited == VISITED_FLAG) {
        return;
    }

    ChartItem *ptr = root;
    do {
        auto grammar_index = ptr->attrs_ptr->grammar_ptr->best_cfg_ptr->shrg_index;
        ptr->rule_ptr = shrg_rules[grammar_index];
        ptr->shrg_index = grammar_index;

        ptr->rule_visited = VISITED_FLAG;
        for (ChartItem *child : ptr->children) {
            addRulePointer_test(child, shrg_rules);
        }
        ptr = ptr->next_ptr;
    } while (ptr != root);
}

void print_chartitem_list(ChartItem *root) {
    ChartItem *ptr = root;
    do {
        std::cout << ptr->shrg_index << " ";
        ptr = ptr->next_ptr;
    }while (ptr != root);
    std::cout << std::endl;
}