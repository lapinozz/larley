#pragma once

#include <any>
#include <functional>
#include <vector>

#include "parsing-tree.hpp"

namespace larley
{
template <typename Ctx, typename T>
struct OptionalCtxFunction;

template <typename Ctx, typename Ret, typename... Args>
struct OptionalCtxFunction<Ctx, Ret(Args... x)>
{
    using type = std::function<Ret(Args..., Ctx&)>;
};
template <typename Ret, typename... Args>
struct OptionalCtxFunction<void, Ret(Args...)>
{
    using type = std::function<Ret(Args...)>;
};

template <typename Ctx, typename T>
using OptionalCtxFunction_t = OptionalCtxFunction<Ctx, T>::type;

template <typename ParserTypes>
class Semantics
{
    using Ctx = ParserTypes::Ctx;
    using Src = ParserTypes::Src;

  public:
    struct SemanticValue : std::any
    {
        using any::any;

        Src src;

        template <typename T>
        T& as()
        {
            return std::any_cast<T&>(*this);
        }

        template <typename T>
        const T& as() const
        {
            return std::any_cast<T>(*this);
        }
    };

    using SemanticValues = std::vector<SemanticValue>;
    using SemanticAction = OptionalCtxFunction_t<Ctx, SemanticValue(SemanticValues&)>;

    std::vector<SemanticAction> actions;

    void setAction(std::size_t id, SemanticAction action)
    {
        if (actions.size() <= id)
        {
            actions.resize(id + 1);
        }

        actions[id] = action;
    }

    using TerminalHandler = OptionalCtxFunction_t<Ctx, SemanticValue(Src)>;

    TerminalHandler terminalHandler;
};

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