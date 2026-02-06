#pragma once

#include "../graph_parser/generator.hpp"
#include "../graph_parser/edsgraph.hpp"
#include "../manager.hpp"
namespace shrg {
namespace em{
using ItemList = std::vector<ChartItem*>;
class EM_DATA_PROCESSOR {
  public:
    EM_DATA_PROCESSOR(std::vector<EdsGraph> &graphs, shrg::Context *context);

    void parseAllGraphs();
    ItemList& getForests();
    void addParentPointer(int index);

  protected:
    static const int VISITED;
    Context *context;
    std::vector<EdsGraph> &graphs;
    std::vector<ChartItem*> forests;
    void addParentPointer(ChartItem *root, int level);
    void addParentPointerOptimized(ChartItem *root, int level);
};
}
}