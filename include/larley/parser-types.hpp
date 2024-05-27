#pragma once

#include <functional>
#include <string_view>

template <typename NonTerminalT, typename TerminalT, typename SrcT = std::string_view, typename CtxT = void>
struct ParserTypes
{
    using Terminal = TerminalT;
    using NonTerminal = NonTerminalT;

    using SrcElement = SrcT;
    using Src = SrcT;

    using Matcher = std::function<int(Src, std::size_t, const Terminal&)>;

    static constexpr bool HasContext = !std::is_same_v<void, CtxT>;
    using Ctx = CtxT;
};