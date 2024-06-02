#pragma once

#include <vector>
#include <deque>

#include "parsing-chart.hpp"

namespace larley
{
    template <typename ParserTypes>
    struct Rule;

    template <typename ParserTypes>
    struct Edge
    {
        std::size_t start{};
        std::size_t end{};
        const Rule<ParserTypes>* rule{};
    };

    template <typename ParserTypes>
    using ParseTree = std::vector<Edge<ParserTypes>>;

    template <typename ParserTypes>
    static ParseTree<ParserTypes> parseTree(const Grammar<ParserTypes>& grammar, const typename ParserTypes::Matcher matcher, const ParseChart<ParserTypes>& chart, typename ParserTypes::Src src)
    {
        const auto& S = chart.S;

        using EdgeSet = std::vector<Edge<ParserTypes>>;
        std::vector<EdgeSet> rchart(S.size());

        for (std::size_t stateIndex = 0; stateIndex < S.size(); stateIndex++)
        {
            for (std::size_t itemIndex = 0; itemIndex < S[stateIndex].size(); itemIndex++)
            {
                const auto& item = S[stateIndex][itemIndex];
                if (!item.isComplete())
                {
                    continue;
                }

                rchart[item.start].emplace_back(item.start, stateIndex, &item.rule);
            }
        }

        for (auto& edgeSet : rchart)
        {
            std::ranges::sort(edgeSet, [](auto& edge1, auto& edge2)
            {
                if (edge2.rule && edge1.rule == edge2.rule)
                {
                    return edge1.end > edge2.end;
                }

                return edge1.rule < edge2.rule; 
            });
        }

        std::vector<Edge<ParserTypes>> result;
        const auto splitEdge = [&](const Edge<ParserTypes>& edge)
        {
            const auto& symbols = edge.rule->symbols;
            const auto symbolCount = symbols.size();
            result.resize(symbolCount);

            const auto iter = [&](this auto const& iter, std::size_t depth, std::size_t start)
            {
                if (depth == symbolCount && start == edge.end)
                {
                    return true;
                }

                if (depth >= symbolCount)
                {
                    return false;
                }

                const auto& symbol = symbols[depth];
                if (auto* nt = std::get_if<0>(&symbol))
                {
                    for (const auto& item : rchart[start])
                    {
                        if (item.rule->product == *nt && iter(depth + 1, item.end))
                        {
                            result[depth] = item;
                            return true;
                        }
                    }
                }
                else if (auto* lt = std::get_if<1>(&symbol))
                {
                    const auto matchLength = matcher(src, start, *lt);
                    if (matchLength > 0)
                    {
                        if (iter(depth + 1, start + matchLength))
                        {
                            result[depth] = {start, start + matchLength};
                            return true;
                        }
                    }
                }

                return false;
            };

            iter(0, edge.start);

            return result;
        };

        ParseTree<ParserTypes> tree;

        const auto iter = [&](this auto const& iter, const Edge<ParserTypes>& edge) -> void
        {
            tree.push_back(edge);

            if (edge.rule)
            {
                for (const auto& subEdge : splitEdge(edge))
                {
                    iter(subEdge);
                }
            }
        };

        for (const auto& edge : rchart[0])
        {
            if (edge.start == 0 && edge.end == rchart.size() - 1 && edge.rule->product == grammar.startSymbol)
            {
                iter(edge);
                break;
            }
        }

        return tree;
    }
 }