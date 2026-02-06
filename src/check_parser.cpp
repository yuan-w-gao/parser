#include "manager.hpp"

#include "graph_parser/parser_tree_base.hpp"

using namespace shrg;

bool CheckTrees(const std::vector<tree::Tree> &trees1, const std::vector<tree::Tree> &trees2) {
    if (trees1.size() != trees2.size())
        return false;
    for (std::size_t i = 0; i < trees1.size(); ++i) {
        auto &tree1 = trees1[i];
        auto &tree2 = trees2[i];
        if (tree1.size() != tree2.size())
            return false;
        for (std::size_t j = 0; j < tree1.size(); ++j) {
            auto ptr1 = tree1[j];
            auto ptr2 = tree2[j];

            auto left1 = ptr1->Left()
                             ? std::find(tree1.begin(), tree1.end(), ptr1->Left()) - tree1.begin()
                             : -1;
            auto right1 = ptr1->Right()
                              ? std::find(tree1.begin(), tree1.end(), ptr1->Right()) - tree1.begin()
                              : -1;
            auto parent1 = ptr1->Parent() ? std::find(tree1.begin(), tree1.end(), ptr1->Parent()) -
                                                tree1.begin()
                                          : -1;
            auto left2 = ptr2->Left()
                             ? std::find(tree2.begin(), tree2.end(), ptr2->Left()) - tree2.begin()
                             : -1;
            auto right2 = ptr2->Right()
                              ? std::find(tree2.begin(), tree2.end(), ptr2->Right()) - tree2.begin()
                              : -1;
            auto parent2 = ptr2->Parent() ? std::find(tree2.begin(), tree2.end(), ptr2->Parent()) -
                                                tree2.begin()
                                          : -1;

            if (left1 != left2 || right1 != right2 || parent1 != parent2)
                return false;
        }
    }
    return true;
}

inline bool operator<(const ChartItem &v1, const ChartItem &v2) {
    using Buffer = std::array<std::uint64_t, //
                              sizeof(EdgeSet) / sizeof(std::uint64_t)>;

    auto &e1 = *reinterpret_cast<const Buffer *>(&v1.edge_set);
    auto &e2 = *reinterpret_cast<const Buffer *>(&v2.edge_set);
    return e1 < e2 && v1.boundary_node_mapping.m8 < v2.boundary_node_mapping.m8;
}

std::tuple<int, int> ItemToKey(const ChartItem *chart_item_ptr, //
                               const std::vector<SHRG> &grammars,
                               const std::vector<tree::Tree> &trees) {
    auto *node_ptr = chart_item_ptr->attrs_ptr;
    int grammar_index = node_ptr->grammar_ptr - grammars.data();

    const tree::Tree &tree_nodes = trees[grammar_index];
    int node_index = std::find(tree_nodes.begin(), tree_nodes.end(), node_ptr) - tree_nodes.begin();

    return {grammar_index, node_index};
}

void CheckForest(const EdsGraph &graph, ChartItem *ptr, uint64_t &count);

void CheckItem(const EdsGraph &graph, ChartItem *ptr, uint64_t &count) {
    if (ptr->edge_set.none() || ptr->status == ChartItem::kVisited) // empty or checked
        return;

    ptr->status = ChartItem::kVisited;
    ++count;

    NodeMapping mapping;
    NodeSet boundary_nodes_of_eds, matched_nodes;
    for (auto &edge : graph.edges)
        if (ptr->edge_set[edge.index])
            for (auto node_ptr : edge.linked_nodes)
                matched_nodes[node_ptr->index] = true;

    for (auto &node : graph.nodes)
        if (matched_nodes[node.index])
            for (auto edge_ptr : node.linked_edges)
                if (!ptr->edge_set[edge_ptr->index])
                    boundary_nodes_of_eds.set(node.index);

    auto item_ptr = static_cast<tree::TreeNodeBase *>(ptr->attrs_ptr);
    if (!item_ptr) { // terminal subgraph
        ASSERT_ERROR(ptr->edge_set.count() == 1, "Invalid terminal 1");
        auto &edge = graph.edges[ptr->edge_set._Find_first()];
        int node_index = 0;
        for (auto node_ptr : edge.linked_nodes)
            if (boundary_nodes_of_eds[node_ptr->index])
                ASSERT_ERROR(ptr->boundary_node_mapping[node_index++] == node_ptr->index + 1,
                             "Invalid terminal 2");
        return;
    }

    if (!item_ptr->Parent()) { // passive
        mapping.m8.fill(0);
        int external_node_index = 0;

        auto grammar_ptr = item_ptr->grammar_ptr;
        for (auto *node_ptr : grammar_ptr->external_nodes) {
            int index_of_node_in_eds = ptr->boundary_node_mapping[external_node_index++];
            ASSERT_ERROR(index_of_node_in_eds > 0, "Invalid subgraph");
            mapping[node_ptr->index] = index_of_node_in_eds;
        }
    } else
        mapping = ptr->boundary_node_mapping;

    for (uint i = 0; i < mapping.size(); ++i)
        if (mapping[i] > 0) {
            ASSERT_ERROR(boundary_nodes_of_eds[mapping[i] - 1], "Invalid mapping 1");
            boundary_nodes_of_eds[mapping[i] - 1] = false;
            ASSERT_ERROR(item_ptr->boundary_nodes[i] == 1, "Mismatch mapping 1");
        } else
            ASSERT_ERROR(item_ptr->boundary_nodes[i] != 1, "Mismatch mapping 2");

    // std::cout << is_passive << '\n';
    // debug::PrintLn(graph, *ptr);
    // debug::PrintLn(graph, *ptr->left_ptr, item_ptr->covered_edge_ptr, 1);
    // debug::PrintLn(graph, *ptr->right_ptr, nullptr, 1);
    ASSERT_ERROR(boundary_nodes_of_eds.none(), "Invalid mapping 2");

    ASSERT_ERROR((ptr->left_ptr->edge_set & ptr->right_ptr->edge_set).none(), "Not disjoint");
    ASSERT_ERROR((ptr->left_ptr->edge_set | ptr->right_ptr->edge_set) == ptr->edge_set,
                 "Invalid merge")

    if (item_ptr->Right()) { // binary
        CheckForest(graph, ptr->left_ptr, count);
        CheckForest(graph, ptr->right_ptr, count);
    } else { // unary
        CheckForest(graph, ptr->right_ptr, count);
        CheckForest(graph, ptr->left_ptr, count);
    }
}

void CheckForest(const EdsGraph &graph, ChartItem *ptr, uint64_t &count) {
    CheckItem(graph, ptr, count);

    auto current_ptr = ptr->next_ptr;
    if (current_ptr) {
        while (current_ptr != ptr) {
            ASSERT_ERROR(*ptr == *current_ptr, "Mismatch subgraph");

            CheckItem(graph, current_ptr, count);
            current_ptr = current_ptr->next_ptr;
        }
    }
}

int main(int argc, char *argv[]) {
    auto &manager = Manager::manager;
    manager.Allocate(1);

    if (argc != 4) {
        LOG_ERROR("Usage: test_parser <parser_type> <grammar_path> <graph_path>");
        return 1;
    }

    manager.LoadGrammars(argv[2]);
    manager.LoadGraphs(argv[3]);

    auto &context = manager.contexts[0];
    context->Init(argv[1], false /* verbose */, 100 /* pool_size */);

    for (auto &graph : manager.edsgraphs) {
        if (graph.sentence_id != "wsj00a/20018020")
            continue;

        auto &pool = context->parser->MemoryPool();
        auto code = context->Parse(graph);

        std::cout << graph.sentence_id << ' ';
        if (code == ParserError::kNone) {
            uint64_t count = 0;
            std::cout << "pool size: " << pool.Size() << " (";
            for (std::size_t i = 0; i < pool.Size(); ++i)
                CheckForest(graph, pool[i], count);
            std::cout << count << " checked)\n";
        } else
            std::cout << ToString(code) << '\n';
    }

    return 0;
}
