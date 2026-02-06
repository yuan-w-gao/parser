#include "manager.hpp"
#include "python/find_best_derivation.cpp"

using namespace shrg;

int main(int argc, char *argv[]) {
    auto &manager = Manager::manager;
    manager.Allocate(1);

    if (argc != 4) {
        LOG_ERROR("Usage: shrg_parser <parser_type> <grammar_path> <graph_path>");
        return 1;
    }

    manager.LoadGrammars(argv[2]);
    manager.LoadGraphs(argv[3]);

    auto &context = manager.contexts[0];
    context->Init(argv[1], true /* verbose */, 100 /* pool_size */);
    auto generator = context->parser->GetGenerator();
    for (auto &graph : manager.edsgraphs) {
        auto code = context->Parse(graph);

        std::cout << graph.sentence_id << "\n>>>>> ";
        if (code == ParserError::kNone) {
            //context->Generate();
            ChartItem *root = context->parser->Result();
            auto derivation = shrg::FindBestDerivation(generator, root);
        } else
            std::cout << ToString(code);
        std::cout << std::endl;
    }

    return 0;
}
