#pragma once

#include <optional>

#include "grammar.hpp"
#include "parser-types.hpp"
#include "parsing-chart.hpp"
#include "parsing-error.hpp"
#include "parsing-semantics.hpp"
#include "parsing-tree.hpp"
#include "printer.hpp"
#include "utils.hpp"

namespace larley
{

template <typename T>
struct CtxHolder
{
    T& ctx;
};

template <>
struct CtxHolder<void>
{
};

template <typename ParserTypes>
//struct Parser : CtxHolder<typename ParserTypes::Ctx>
struct Parser
{
    using Src = typename ParserTypes::Src;
    using SemanticValue = typename Semantics<ParserTypes>::SemanticValue;

    Grammar<ParserTypes> grammar;
    ParserTypes::Matcher matcher;
    Semantics<ParserTypes> semantics;

    Src src;

    std::optional<ParseChart<ParserTypes>> chart;
    std::optional<ParseTree<ParserTypes>> tree;
    std::optional<SemanticValue> result;

    std::optional<ParseError<ParserTypes>> error;

    void parseChart()
    {
        chart = ::parseChart(grammar, matcher, src);
    }

    void parseTree()
    {
        assert(chart && "chart is not set");
        assert(chart->matchCount > 0 && "chart has no match");
        tree = ::parseTree(grammar, matcher, *chart, src);
    }

    void parseSemantics()
    {
        assert(tree && "tree is not set");
        result = ::parseSemantics(semantics, *tree, src);
    }

    void parseError()
    {
        assert(chart && "chart is not set");
        if (chart->matchCount == 0 || !chart->completeMatch)
        {
            error = ::parseError(grammar, *chart);
        }
        else 
        {
            error = std::nullopt;
        }
    }

    void printGrammar()
    {
        ::printGrammar(grammar);
    }

    void printChart()
    {
        assert(chart && "chart is not set");
        ::printChart(grammar, *chart);
    }

    void printTree()
    {
        assert(tree && "tree is not set");
        ::printTree(grammar, *tree, src);
    }

    void printError()
    {
        assert(error && "error is not set");
        ::printError(grammar, *error, src);
    }

    SemanticValue parse(Src source, bool acceptPartialMatch = false)
    {
        src = source;

        chart = std::nullopt;
        tree = std::nullopt;
        result = std::nullopt;
        error = std::nullopt;

        parseChart();
        if (chart->matchCount <= 0 || (!acceptPartialMatch && !chart->completeMatch))
        {
            parseError();
            return {};
        }

        parseTree();
        parseSemantics();
        return *result;
    }
};

} // namespace larley