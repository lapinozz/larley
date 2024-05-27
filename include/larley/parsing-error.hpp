#pragma once

#include <vector>

#include "parser.hpp"

namespace larley
{

template <typename ParserTypes>
struct ParseError
{
    struct Prediction
    {
        const typename ParserTypes::Terminal& terminal;

        using Path = std::vector<const Item<ParserTypes>*>;
        Path path;
    };

    std::size_t position{};
    std::vector<Prediction> predictions;
};

namespace impl
{
    void buildPath(const auto& grammar, auto& ParseChart, auto& path)
    {
        const auto& lastItem = *path.back();

        if (lastItem.start == 0 && lastItem.rule.product == grammar.startSymbol)
        {
            return;
        }

        const auto& NT = lastItem.rule.product;

        for (const auto& item : ParseChart.S[lastItem.start])
        {
            if (std::ranges::contains(path, &item))
            {
                continue;
            }

            if (item.isAtSymbol(NT))
            {
                path.push_back(&item);
                buildPath(grammar, ParseChart, path);
                break;
            }
        }
    }
}

template <typename ParserTypes>
ParseError<ParserTypes> parseError(const Grammar<ParserTypes>& grammar, const typename ParseChart<ParserTypes>& ParseChart)
{
    ParseError<ParserTypes> error;

    const auto& S = ParseChart.S;
    if (S.empty())
    {
        return error;
    }

    error.position = S.size() - 1;

    const auto& set = S.back();
    for (const auto& item : set)
    {
        if (item.isComplete())
        {
            continue;
        }

        const auto& symbol = item.rule.symbols[item.dot];

        if (const auto* LT = std::get_if<1>(&symbol))
        {
            auto& prediction = error.predictions.emplace_back(*LT);

            prediction.path.push_back(&item);
            impl::buildPath(grammar, ParseChart, prediction.path);
        }
    }

	return error;
}

} // namespace larley