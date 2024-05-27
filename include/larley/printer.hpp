#pragma once

#include <iostream>
#include <functional>
#include <variant>

#include "utils.hpp"

namespace larley
{

std::size_t findMaximumNonTerminalLenght(const auto& grammar)
{
    std::size_t maximumNonTerminalLength{};
    std::ostringstream ss;
    for (const auto& rule : grammar.rules)
    {
        ss.str("");
        ss << rule.product;
        maximumNonTerminalLength = std::max(maximumNonTerminalLength, ss.view().size());
    }
    return maximumNonTerminalLength;
}

void printTree(const auto& grammar, const auto& tree, const auto& src)
{
    std::cout << "-------- ParseTree --------" << std::endl;

    const auto maximumNonTerminalLength = findMaximumNonTerminalLenght(grammar);

    std::size_t index = 0;
    const auto iter = [&](this const auto& iter, std::size_t depth = 0) -> void
    {
        const auto& edge = tree[index++];

        for (int x = 0; x < depth; x++)
        {
            std::cout << "  ";
        }

        if (edge.rule)
        {
            std::cout << edge.rule->product <<  " ( " << edge.start << ", " << edge.end << ")\n";

            for (int x = 0; x < edge.rule->symbols.size(); x++)
            {
                iter(depth + 1);
            }
        }
        else
        {
            std::cout << '"';
            printUnescaped({src.data() + edge.start, src.data() + edge.end});
            std::cout << "\" ( " << edge.start << ", " << edge.end << ")\n";
        }
    };

    iter();

    std::cout << std::endl << std::endl;
}

void printError(const auto& grammar, const auto& parserError, const auto& src)
{
    std::cout << "-------- Error --------" << std::endl;

    if (parserError.position >= src.size())
    {
        std::cout << "Unexpected end of input " << std::endl;
    }
    else
    {
        std::cout << "Unexcepected character '";
        printUnescaped({src.data() + parserError.position, 1});
        std::cout << "' " << std::endl;
    }

    std::size_t lineCount{};
    std::size_t lastLineStart{};

    for (std::size_t index{}; index < src.size() && index < parserError.position; index++)
    {
        if (src[index] == '\n')
        {
            lineCount++;
            lastLineStart = index + 1;
        }
    }

    const std::size_t column = parserError.position - lastLineStart;

    std::cout << "Line " << lineCount << " column " << column << std::endl;
    printUnescaped({src.data() + lastLineStart, column + 1});
    std::cout << std::endl << std::setw(column + 1) << '^' << std::endl;

    std::cout << std::endl;

    std::cout << "Expected one of the following:" << std::endl;

    const auto maximumNonTerminalLength = findMaximumNonTerminalLenght(grammar);

    for (const auto& prediction : parserError.predictions)
    {
        std::cout << prediction.terminal << " from:" << std::endl;

        for (const auto itemPtr : prediction.path)
        {
            const auto& item = *itemPtr;
            const auto& rule = item.rule;
            std::cout << "    " << std::setw(maximumNonTerminalLength) << rule.product << " ->";

            for (std::size_t z = 0; z < rule.symbols.size(); z++)
            {
                const auto& symbol = rule.symbols[z];

                if (item.dot == z)
                {
                    std::print("\u2022");
                }
                else
                {
                    std::cout << " ";
                }

                if (auto* NT = std::get_if<0>(&symbol))
                {
                    std::cout << *NT;
                }
                else if (auto* LT = std::get_if<1>(&symbol))
                {
                    std::cout << *LT;
                }
            }

            std::cout << " (" << item.start << ")" << std::endl;
        }

        std::cout << std::endl;
    }
}

void printGrammar(const auto& grammar)
{
    std::cout << "-------- Grammar --------" << std::endl;

    const auto maximumNonTerminalLength = findMaximumNonTerminalLenght(grammar);

    for (const auto& rule : grammar.rules)
    {
        std::cout << std::setw(maximumNonTerminalLength) << rule.product << " ->";

        for (const auto& symbol : rule.symbols)
        {
            std::cout << " ";
            if (auto* NT = std::get_if<0>(&symbol))
            {
                std::cout << *NT;
            }
            else if (auto* LT = std::get_if<1>(&symbol))
            {
                std::cout << *LT;
            }
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void printChart(const auto& grammar, const auto& chart)
{
    std::cout << "-------- Charts --------" << std::endl;

    const auto maximumNonTerminalLength = findMaximumNonTerminalLenght(grammar);

    const auto& S = chart.S;
    for (std::size_t x = 0; x < S.size(); x++)
    {
        std::cout << "Chart: " << x << std::endl;
        const auto& set = S[x];
        for (std::size_t y = 0; y < set.size(); y++)
        {
            const auto& item = set[y];
            const auto& rule = item.rule;
            std::cout << std::setw(maximumNonTerminalLength) << rule.product << " ->";

            for (std::size_t z = 0; z < rule.symbols.size(); z++)
            {
                const auto& symbol = rule.symbols[z];

                if (item.dot == z)
                {
                    std::print("\u2022");
                }
                else
                {
                    std::cout << " ";
                }

                if (auto* NT = std::get_if<0>(&symbol))
                {
                    std::cout << *NT;
                }
                else if (auto* LT = std::get_if<1>(&symbol))
                {
                    std::cout << *LT;
                }
            }

            if (item.dot == rule.symbols.size())
            {
                std::print("\u2022");
            }

            std::cout << " (" << item.start << ")" << std::endl;
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
}

} // namespace larley