#pragma once

#include "grammar.hpp"
#include "utils.hpp"

namespace larley
{

template <typename ParserTypes>
struct Item
{
    const Rule<ParserTypes>& rule;
    std::size_t start;
    std::size_t dot;

    bool operator==(const Item& other) const
    {
        return start == other.start && dot == other.dot && &rule == &other.rule;
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

        const auto& ruleSymbol = rule.symbols[dot];
        return ruleSymbol.index() == 0 && std::get<0>(ruleSymbol) == symbol;
    }

    struct Hash
    {
        std::size_t operator()(const Item& item) const noexcept
        {
            return std::hash<std::size_t>{}((item.start << 32) | (item.rule.id << 16) | (item.dot << 0));
        }
    };
};

template <typename ParserTypes>
struct StateSet : std::vector<Item<ParserTypes>>
{
    std::unordered_set<Item<ParserTypes>, typename Item<ParserTypes>::Hash> added;
};

template <typename ParserTypes>
struct ParseChart
{
    std::vector<StateSet<ParserTypes>> S;
    bool completeMatch = false;
    std::size_t matchCount{};
};

template<typename ParserTypes>
static ParseChart<ParserTypes> parseChart(const Grammar<ParserTypes>& grammar, const typename ParserTypes::Matcher& matcher, typename ParserTypes::Src src)
{
    ParseChart<ParserTypes> result;

    auto& S = result.S;
    S.resize(src.size() + 1);

    std::vector<bool> ruleStarted;
    ruleStarted.resize(grammar.rules.size());

    const auto addItem = [&](auto& set, Item<ParserTypes>&& item)
    {
        if constexpr (true)
        {
            if (item.dot == 0)
            {
                if (ruleStarted[item.rule.id])
                {
                    return;
                }

                ruleStarted[item.rule.id] = true;
            }

            if (set.added.insert(item).second)
            {
                set.push_back(std::move(item));
            }
        }
        else
        {
            if (std::find(set.begin(), set.end(), item) == set.end())
            {
                set.push_back(std::move(item));
            }
        }
    };

    std::unordered_map<typename ParserTypes::NonTerminal, std::vector<const Rule<ParserTypes>*>> productToRules;
    for (const auto& rule : grammar.rules)
    {
        productToRules[rule.product].push_back(&rule);
    }

    for (const auto* rule : productToRules.at(grammar.startSymbol))
    {
        addItem(S[0], {*rule, 0, 0});
    }

    for (std::size_t stateIndex = 0; stateIndex < S.size(); stateIndex++)
    {
        auto& set = S[stateIndex];

        ruleStarted.resize(0);
        ruleStarted.resize(grammar.rules.size());

        for (std::size_t itemIndex = 0; itemIndex < set.size(); itemIndex++)
        {
            const auto item = set[itemIndex];

            if (item.isComplete())
            {
                const auto& potentialSet = S[item.start];
                for (std::size_t potentialIndex = 0; potentialIndex < potentialSet.size(); potentialIndex++)
                {
                    const auto& potentialItem = potentialSet[potentialIndex];
                    if (potentialItem.isAtSymbol(item.rule.product))
                    {
                        addItem(set, potentialItem.advanced());
                    }
                }

                continue;
            }

            const auto& symbol = item.rule.symbols[item.dot];

            if (auto* nt = std::get_if<0>(&symbol))
            {
                if (grammar.nullables.contains(*nt))
                {
                    addItem(set, item.advanced());
                }

                const auto it = productToRules.find(*nt);
                if (it != productToRules.end())
                {
                    for (const auto* rule : it->second)
                    {
                        addItem(set, {*rule, stateIndex, 0});
                    }
                }
            }
            else if (auto* lt = std::get_if<1>(&symbol))
            {
                const auto matchLength = matcher(src, stateIndex, *lt);
                if (matchLength > 0)
                {
                    addItem(S[stateIndex + matchLength], item.advanced());
                }
            }
        }
    }

    std::size_t setCount{};
    for (std::size_t index{}; index < S.size(); index++)
    {
        if (!S.empty())
        {
            setCount = index + 1;
        }
    }
    S.resize(setCount);

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