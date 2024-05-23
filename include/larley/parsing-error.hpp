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

        using Path = std::vector<const typename Parser<ParserTypes>::Item*>;
        Path path;
    };

    std::size_t position{};
    std::vector<Prediction> predictions;
};

namespace impl
{
    void buildPath(auto& inputs, auto& parseResult, auto& path)
    {
        const auto& lastItem = *path.back();

        if (lastItem.start == 0 && lastItem.rule.product == inputs.grammar.startSymbol)
        {
            return;
        }

        const auto& NT = lastItem.rule.product;

        for (const auto& item : parseResult.S[lastItem.start])
        {
            if (std::ranges::contains(path, &item))
            {
                continue;
            }

            if (item.isAtSymbol(NT))
            {
                path.push_back(&item);
                buildPath(inputs, parseResult, path);
                break;
            }
        }
    }
}

template <typename ParserTypes>
auto makeParseError(const ParserInputs<ParserTypes>& inputs, const typename Parser<ParserTypes>::ParseResult& parseResult)
{
    ParseError<ParserTypes> error;

    const auto& S = parseResult.S;
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
            impl::buildPath(inputs, parseResult, prediction.path);
        }
    }

	return error;
}

} // namespace larley