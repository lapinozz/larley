#pragma once

#include <vector>

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
                    return edge2.end < edge1.end;
                }

                return edge1.rule < edge2.rule; });
        }

        const auto splitEdge = [&](Edge<ParserTypes> edge)
        {
            const auto& symbols = edge.rule->symbols;
            std::vector<Edge<ParserTypes>> result(symbols.size());

            const auto iter = [&](this auto const& iter, std::size_t depth, std::size_t start)
            {
                if (depth == symbols.size() && start == edge.end)
                {
                    return true;
                }

                if (depth >= symbols.size())
                {
                    return false;
                }

                const auto& symbol = symbols[depth];

                const bool found = std::visit(
                    overloaded{
                        [&](const ParserTypes::NonTerminal& nt)
                        {
                            for (const auto& item : rchart[start])
                            {
                                if (item.rule->product == nt && iter(depth + 1, item.end))
                                {
                                    result[depth] = item;
                                    return true;
                                }
                            }

                            return false;
                        },
                        [&](const ParserTypes::Terminal& lt)
                        {
                            const auto matchLength = matcher(src, start, lt);
                            if (matchLength > 0)
                            {
                                if (iter(depth + 1, start + matchLength))
                                {
                                    result[depth] = {start, start + matchLength};
                                    return true;
                                }
                            }

                            return false;
                        }},
                    symbol);

                return found;
            };

            iter(0, edge.start);

            return result;
        };

        ParseTree<ParserTypes> tree;

        const auto iter = [&](this auto const& iter, Edge<ParserTypes> edge) -> void
        {
            tree.push_back(edge);

            if (edge.rule)
            {
                const auto split = splitEdge(edge);
                for (const auto& subEdge : split)
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