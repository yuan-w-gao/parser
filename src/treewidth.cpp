#include <iomanip>

#include "manager.hpp"

#include "graph_parser/tree_decomposer.hpp"

using namespace shrg;
using namespace shrg::tree;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        LOG_ERROR("Usage: " << argv[0] << "  <grammar-path>");
        return 1;
    }

    auto &manager = Manager::manager;
    manager.LoadGrammars(argv[1]);

    TreeDecomposerTpl<TreeNodeBase, MinimumWidthDecomposer> decomposer;
    utils::MemoryPool<TreeNodeBase> tree_nodes;
    Tree tree;

    decomposer.SetPool(&tree_nodes);

    int count = 0, lexicon_count = 0;

#define DEFINE(suffix) int min_##suffix = 1000, max_##suffix = 0, avg_##suffix = 0;

#define COLLECT(value, suffix)                                                                     \
    {                                                                                              \
        int _value = (value);                                                                      \
        min_##suffix = std::min(min_##suffix, _value);                                             \
        max_##suffix = std::max(max_##suffix, _value);                                             \
        avg_##suffix += _value;                                                                    \
    }

#define OUTPUT(suffix, total)                                                                      \
    {                                                                                              \
        std::cout << "| " << std::setw(25) << #suffix << " | " << std::setw(10) << min_##suffix    \
                  << " | " << std::setw(10) << max_##suffix << " | " << std::setw(10)              \
                  << (double)avg_##suffix / total << " |\n";                                       \
    }

    DEFINE(width);
    DEFINE(num_nodes);
    DEFINE(num_terminals);
    DEFINE(lexicon_width);
    DEFINE(lexicon_num_nodes);
    DEFINE(lexicon_num_terminals);
    for (auto &grammar : manager.grammars) {
        if (grammar.IsEmpty())
            continue;

        tree.clear();
        decomposer.Decompose(tree, grammar);

        int width = std::max(tree[0]->Width(), 0);
        if (!grammar.nonterminal_edges.empty()) {
            count++;
            COLLECT(width, width);
            COLLECT(grammar.fragment.nodes.size(), num_nodes);
            COLLECT(grammar.terminal_edges.size(), num_terminals);
        } else {
            lexicon_count++;
            COLLECT(width, lexicon_width);
            COLLECT(grammar.fragment.nodes.size(), lexicon_num_nodes);
            COLLECT(grammar.terminal_edges.size(), lexicon_num_terminals);
        }
    }

    std::cout << "nonlexicon = " << count << " lexicon = " << lexicon_count << '\n';
    std::cout << "| " << std::setw(25) << ""
              << " | " << std::setw(10) << "min"
              << " | " << std::setw(10) << "max"
              << " | " << std::setw(10) << "avg"
              << " |\n";
    OUTPUT(width, count);
    OUTPUT(num_nodes, count);
    OUTPUT(num_terminals, count);
    OUTPUT(lexicon_width, lexicon_count);
    OUTPUT(lexicon_num_nodes, lexicon_count);
    OUTPUT(lexicon_num_terminals, lexicon_count);
    return 0;
}
