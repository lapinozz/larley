#pragma once

#include "grammar.hpp"
#include "utils.hpp"

namespace larley
{

template <typename ParserTypes>
struct Item
{
    const Rule<ParserTypes>& rule;
    std::size_t start{};
    std::size_t dot{};

    bool operator==(const Item& other) const
    {
        return &rule == &other.rule && start == other.start && dot == other.dot;
    }

    Item advanced() const
    {
        return {rule, start, dot + 1};
    }

    bool isComplete() const
    {
        return dot >= rule.symbols.size();
    }

    bool isAtSymbol(ParserTypes::NonTerminal symbol) const
    {
        if (isComplete())
        {
            return false;
        }

        const auto ruleSymbol = rule.symbols[dot];
        return ruleSymbol.index() == 0 && std::get<0>(ruleSymbol) == symbol;
    }
};

template <typename ParserTypes>
using StateSet = std::vector<Item<ParserTypes>>;

template <typename ParserTypes>
struct ParseChart
{
    std::vector<StateSet<ParserTypes>> S;
    bool completeMatch = false;
    std::size_t matchCount{};
};

template<typename ParserTypes>
static ParseChart<ParserTypes> parseChart(Grammar<ParserTypes>& grammar, typename ParserTypes::Matcher& matcher, typename ParserTypes::Src src)
{
    ParseChart<ParserTypes> result;

    auto& S = result.S;

    auto addItem = [&S](std::size_t stateIndex, Item<ParserTypes> item)
    {
        if (S.size() <= stateIndex)
        {
            S.resize(stateIndex + 1);
        }

        auto& set = S[stateIndex];
        if (std::find(set.begin(), set.end(), item) == set.end())
        {
            set.push_back(item);
        }
    };

    S.emplace_back();

    for (const auto& rule : grammar.rules)
    {
        if (rule.product == grammar.startSymbol)
        {
            addItem(0, {rule});
        }
    }

    for (std::size_t stateIndex = 0; stateIndex < S.size(); stateIndex++)
    {
        for (std::size_t itemIndex = 0; itemIndex < S[stateIndex].size(); itemIndex++)
        {
            const auto item = S[stateIndex][itemIndex];

            if (item.isComplete())
            {
                for (std::size_t potentialIndex = 0; potentialIndex < S[item.start].size(); potentialIndex++)
                {
                    const auto& potentialItem = S[item.start][potentialIndex];

                    if (potentialItem.isComplete())
                    {
                        continue;
                    }

                    const auto symbol = potentialItem.rule.symbols[potentialItem.dot];
                    if (potentialItem.isAtSymbol(item.rule.product))
                    {
                        addItem(stateIndex, potentialItem.advanced());
                    }
                }

                continue;
            }

            const auto symbol = item.rule.symbols[item.dot];
            std::visit(
                overloaded{
                    [&](const ParserTypes::NonTerminal& nt)
                    {
                        for (const auto& rule : grammar.rules)
                        {
                            if (rule.product == nt)
                            {
                                addItem(stateIndex, {rule, stateIndex});

                                if (grammar.nullables.contains(rule.product))
                                {
                                    addItem(stateIndex, item.advanced());
                                }
                            }
                        }
                    },
                    [&](const ParserTypes::Terminal& lt)
                    {
                        const auto matchLength = matcher(src, stateIndex, lt);
                        if (matchLength > 0)
                        {
                            addItem(stateIndex + matchLength, item.advanced());
                        }
                    }},
                symbol);
        }
    }

    if (S.size() == src.size() + 1)
    {
        result.completeMatch = true;
    }

    for (const auto& item : S.back())
    {
        if (item.start == 0 && item.isComplete() && item.rule.product == grammar.startSymbol)
        {
            result.matchCount++;
        }
    }

    return result;
}

} // namespace larley