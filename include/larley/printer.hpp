#pragma once

#include <iostream>
#include <functional>
#include <variant>

namespace larley
{

template <typename PrinterTypes>
struct Printer
{
    std::size_t maximumNonTerminalLength{};

    std::function<void(std::ostream&, const typename PrinterTypes::Terminal&)> printTerminal;
    std::function<void(std::ostream&, const typename PrinterTypes::NonTerminal&)> printNonTerminal;
};

void printTree(const auto& printer, const auto& inputs, const auto& tree)
{
    std::cout << "-------- Tree --------" << std::endl;

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
            printer.printNonTerminal(std::cout, edge.rule->product);
            std::cout <<  " ( " << edge.start << ", " << edge.end << ")\n";

            for (int x = 0; x < edge.rule->symbols.size(); x++)
            {
                iter(depth + 1);
            }
        }
        else
        {
            std::cout << '"' << std::string_view(inputs.src.data() + edge.start, inputs.src.data() + edge.end) << "\" ( " << edge.start << ", " << edge.end << ")\n";
        }
    };

    iter();

    std::cout << std::endl
              << std::endl;
}

void printError(const auto& printer, const auto& inputs, const auto& parserError)
{
    std::cout << "-------- Error --------" << std::endl;

    if (parserError.position >= inputs.src.size())
    {
        std::cout << "Unexpected end of input " << std::endl;
    }
    else
    {
        std::cout << "Unexcepected character '" << inputs.src[parserError.position] << "' " << std::endl;
    }

    std::size_t lineCount{};
    std::size_t lastLineStart{};

    for (std::size_t index{}; index < inputs.src.size() && index < parserError.position; index++)
    {
        if (inputs.src[index] == '\n')
        {
            lineCount++;
            lastLineStart = index + 1;
        }
    }

    const std::size_t column = parserError.position - lastLineStart;

    std::cout << "Line " << lineCount << " column " << column << std::endl;
    std::cout << std::string_view(inputs.src.data() + lastLineStart, column + 1) << std::endl;
    std::cout << std::setw(column + 1) << '^' << std::endl;

    std::cout << std::endl;

    std::cout << "Expected one of the following:" << std::endl;

    for (const auto& prediction : parserError.predictions)
    {
        printer.printTerminal(std::cout, prediction.terminal);
        std::cout << " from:" << std::endl;

        for (const auto itemPtr : prediction.path)
        {
            const auto& item = *itemPtr;
            const auto& rule = item.rule;
            std::cout << "    " << std::setw(printer.maximumNonTerminalLength);
            printer.printNonTerminal(std::cout, rule.product);
            std::cout << " ->";

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
                    printer.printNonTerminal(std::cout, *NT);
                }
                else if (auto* LT = std::get_if<1>(&symbol))
                {
                    printer.printTerminal(std::cout, *LT);
                }
            }

            std::cout << " (" << item.start << ")" << std::endl;
        }

        std::cout << std::endl;
    }
}

void printGrammar(const auto& printer, const auto& grammar)
{
    std::cout << "-------- Grammar --------" << std::endl;

    for (const auto& rule : grammar.rules)
    {
        std::cout << std::setw(printer.maximumNonTerminalLength);
        printer.printNonTerminal(std::cout, rule.product);
        std::cout << " ->";

        for (const auto& symbol : rule.symbols)
        {
            std::cout << " ";
            if (auto* NT = std::get_if<0>(&symbol))
            {
                printer.printNonTerminal(std::cout, *NT);
            }
            else if (auto* LT = std::get_if<1>(&symbol))
            {
                printer.printTerminal(std::cout, *LT);
            }
        }

        std::cout << std::endl;
    }

    std::cout << std::endl;
}

void printChart(const auto& printer, const auto& inputs, const auto& parseResult)
{
    std::cout << "-------- Charts --------" << std::endl;

    const auto& S = parseResult.S;
    for (std::size_t x = 0; x < S.size(); x++)
    {
        std::cout << "Chart: " << x << std::endl;
        const auto& set = S[x];
        for (std::size_t y = 0; y < set.size(); y++)
        {
            const auto& item = set[y];
            const auto& rule = item.rule;
            std::cout << std::setw(printer.maximumNonTerminalLength);
            printer.printNonTerminal(std::cout, rule.product);
            std::cout << " ->";

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
                    printer.printNonTerminal(std::cout, *NT);
                }
                else if (auto* LT = std::get_if<1>(&symbol))
                {
                    printer.printTerminal(std::cout, *LT);
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