#pragma once

#include <variant>
#include <string>
#include <vector>
#include <array>
#include <ranges>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "parser.hpp"

namespace larley
{

template <typename ParserTypes>
struct TreeBuilder
{
    struct Edge
    {
        std::size_t start{};
        std::size_t end{};
        const Rule<ParserTypes>* rule{};
    };

    using Tree = std::vector<Edge>;

    static Tree buildTree(const ParserInputs<ParserTypes>& inputs, const typename Parser<ParserTypes>::ParseResult& result)
    {
        const auto& src = inputs.src;
        const auto& matcher = inputs.matcher;
        const auto& grammar = inputs.grammar;

        const auto& S = result.S;

        using EdgeSet = std::vector<Edge>;
        std::vector<EdgeSet> chart(S.size());

        for (std::size_t stateIndex = 0; stateIndex < S.size(); stateIndex++)
        {
            for (std::size_t itemIndex = 0; itemIndex < S[stateIndex].size(); itemIndex++)
            {
                const auto& item = S[stateIndex][itemIndex];
                if (!item.isComplete())
                {
                    continue;
                }

                chart[item.start].emplace_back(item.start, stateIndex, &item.rule);
            }
        }

        for (auto& edgeSet : chart)
        {
            std::ranges::sort(edgeSet, [](auto& edge1, auto& edge2)
                              {
                if (edge2.rule && edge1.rule == edge2.rule)
                {
                    return edge2.end < edge1.end;
                }

                return edge1.rule < edge2.rule; });
        }

        const auto splitEdge = [&](Edge edge)
        {
            const auto& symbols = edge.rule->symbols;
            std::vector<Edge> result(symbols.size());

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
                            for (const auto& item : chart[start])
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

        Tree tree;

        const auto iter = [&](this auto const& iter, Edge edge) -> void
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

        for (const auto& edge : chart[0])
        {
            if (edge.start == 0 && edge.end == chart.size() - 1 && edge.rule->product == grammar.startSymbol)
            {
                iter(edge);
                break;
            }
        }

        return tree;
    }
};

template <typename ParserTypes>
auto buildTree(const ParserInputs<ParserTypes>& inputs, const typename Parser<ParserTypes>::ParseResult& result)
{
    return TreeBuilder<ParserTypes>::buildTree(inputs, result);
}

} // namespace larley