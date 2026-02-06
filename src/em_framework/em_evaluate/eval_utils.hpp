//
// Created by Yuan Gao on 06/12/2024.
//
#ifndef SHRG_GRAPH_PARSER_EVAL_UTILS_HPP
#define SHRG_GRAPH_PARSER_EVAL_UTILS_HPP

#include "../em_base.hpp"

using namespace shrg;

bool isLexicalRule(const SHRG *rule){
    const auto &edges = rule->fragment.edges;
    return std::all_of(edges.begin(), edges.end(), [](const SHRG::Edge &e){return e.is_terminal; });
}

#endif // SHRG_GRAPH_PARSER_EVAL_UTILS_HPP
