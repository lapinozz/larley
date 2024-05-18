#pragma once

#include "parser.hpp"
#include "tree-builder.hpp"

namespace larley
{

template <typename ParserTypes>
auto applySemantics(const ParserInputs<ParserTypes>& inputs, const typename TreeBuilder<ParserTypes>::Tree& tree)
{
    using SemanticValue = Semantics<ParserTypes>::SemanticValue;
    using SemanticValues = Semantics<ParserTypes>::SemanticValues;

    std::size_t index = 0;
    const auto iterate = [&](const auto& self) -> SemanticValue
    {
        const auto& edge = tree[index++];
        if (edge.rule)
        {
            SemanticValues values;
            for (int x = 0; x < edge.rule->symbols.size(); x++)
            {
                values.push_back(self(self));
            }

            const auto& action = inputs.semantics.actions[edge.rule->id];
            if (action)
            {
                return action(values);
            }
            else if (values.size() > 0)
            {
                return values[0];
            }
            else
            {
                return {};
            }
        }
        else
        {
            return inputs.src.subspan(edge.start, edge.end - edge.start);
        }
    };

    return iterate(iterate);
}

} // namespace larley