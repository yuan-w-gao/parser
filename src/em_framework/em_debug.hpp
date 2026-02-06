//
// Created by Yuan Gao on 05/06/2024.
//
#pragma once

#include "em_utils.hpp"
#include "em_types.hpp"
#include "../manager.hpp"

namespace shrg {
namespace em_debug {
    struct ForestInfo{
        int num_nodes;
        int max_depth;
        std::unordered_map<int, int> width_at_depth;

        ForestInfo(): num_nodes(0), max_depth(0) {}
        void TraverseForest(ChartItem *root_ptr, ForestInfo &info, Generator *generator,int depth=0);
        ForestInfo GetForestInfo(Generator* generator, ChartItem* root_ptr);
        void PrintForestInfo(const ForestInfo &info);
    };

    ChartItem* deepCopyChartItem(ChartItem* root);
    bool compareChartItems(ChartItem* root1, ChartItem* root2);
    void deleteChartItem(ChartItem* root);
}
}