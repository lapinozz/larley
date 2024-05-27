#pragma once

#include "parser.hpp"
#include "grammar.hpp"

namespace larley
{

template <typename ParserTypes>
auto parseSemantics(const Semantics<ParserTypes>& semantics, const ParseTree<ParserTypes> tree, typename ParserTypes::Src src)
{
    using SemanticValue = Semantics<ParserTypes>::SemanticValue;
    using SemanticValues = Semantics<ParserTypes>::SemanticValues;

    std::size_t index = 0;
    const auto iterate = [&](const auto& self) -> SemanticValue
    {
        SemanticValue value;

        const auto& edge = tree[index++];
        if (edge.rule)
        {
            SemanticValues values;
            for (int x = 0; x < edge.rule->symbols.size(); x++)
            {
                if (!edge.rule->isDiscarded(x))
                {
                    values.push_back(self(self));
                }
                else
                {
                    self(self);
                }
            }

            const auto& action = semantics.actions[edge.rule->id];
            if (action)
            {
                value = action(values);
            }
            else if (values.size() > 0)
            {
                value = std::move(values[0]);
            }
        }

        value.src = {src.data() + edge.start, src.data() + edge.end};
        return value;
    };

    return iterate(iterate);
}

} // namespace larley