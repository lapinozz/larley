#pragma once

#include <any>
#include <functional>
#include <vector>

#include "parsing-tree.hpp"

namespace larley
{

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
    struct SemanticAction : std::function<SemanticValue(SemanticValues&, Ctx*)>
    {
        using std::function<SemanticValue(SemanticValues&, Ctx*)>::function;

        template <typename F>
        requires std::invocable<F, SemanticValues&> && !std::is_void_v<std::invoke_result_t<F, SemanticValues&>>
        SemanticAction(F f) : std::function<SemanticValue(SemanticValues&, Ctx*)>{[f](SemanticValues& values, Ctx*){return f(values);}}
        {

        }

        template <typename F>
        requires std::invocable<F, SemanticValues&> && std::is_void_v<std::invoke_result_t<F, SemanticValues&>>
        SemanticAction(F f) : std::function<SemanticValue(SemanticValues&, Ctx*)>{[f](SemanticValues& values, Ctx*) -> SemanticValue {f(values); return {};}}
        {

        }

        template <typename F>
        requires std::invocable<F, SemanticValues&, Ctx*> && std::is_void_v<std::invoke_result_t<F, SemanticValues&, Ctx*>>
        SemanticAction(F f) : std::function<SemanticValue(SemanticValues&, Ctx*)>{[f](SemanticValues& values, Ctx* ctx) -> SemanticValue {f(values, ctx); return {};}}
        {

        }
    };

    std::vector<SemanticAction> actions;

    void setAction(std::size_t id, SemanticAction action)
    {
        if (actions.size() <= id)
        {
            actions.resize(id + 1);
        }

        actions[id] = std::move(action);
    }
};

template <typename ParserTypes>
auto parseSemantics(const Semantics<ParserTypes>& semantics, const ParseTree<ParserTypes> tree, typename ParserTypes::Src src, typename ParserTypes::Ctx* ctx)
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
                value = action(values, ctx);
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