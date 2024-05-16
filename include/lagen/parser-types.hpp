#pragma once

#include <functional>

template <typename NonTerminalT, typename TerminalT, typename SrcElementT = char, typename CtxT = void>
struct ParserTypes
{
    using Terminal = TerminalT;
    using NonTerminal = NonTerminalT;

    using SrcElement = SrcElementT;
    using Src = std::span<const SrcElement>;

    using Matcher = std::function<int(Src, std::size_t, const Terminal&)>;

    static constexpr bool HasContext = !std::is_same_v<void, CtxT>;
    using Ctx = CtxT;
};