#pragma once

#include <functional>

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

        template <typename T>
        T as()
        {
            return std::any_cast<T>(*this);
        }

        template <typename T>
        T as() const
        {
            return std::any_cast<T>(*this);
        }
    };

    using SemanticValues = std::vector<SemanticValue>;
    using SemanticAction = OptionalCtxFunction_t<Ctx, SemanticValue(const SemanticValues&)>;

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

} // namespace larley