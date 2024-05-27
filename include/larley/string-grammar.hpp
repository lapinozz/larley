#pragma once

#include <array>
#include <regex>
#include <string_view>
#include <variant>
#include <vector>
#include <optional>
#include <sstream>

#include "parser.hpp"
#include "utils.hpp"

namespace larley
{

namespace StringGrammar
{

using Str = std::string;

struct SavedRegex : public std::regex
{
    SavedRegex(std::string pattern) : std::regex(pattern), pattern{ std::move(pattern) }
    {

    }

    std::string pattern;
};

using LiteralTerminalSymbol = Str;

using ChoiceTerminalSymbol = std::vector<LiteralTerminalSymbol>;
using RangeTerminalSymbol = std::pair<LiteralTerminalSymbol, LiteralTerminalSymbol>;
using RegexTerminalSymbol = SavedRegex;

using TerminalSymbol = std::variant<LiteralTerminalSymbol, ChoiceTerminalSymbol, RangeTerminalSymbol, RegexTerminalSymbol>;

int match(std::string_view src, size_t index, const TerminalSymbol& symbol)
{
    return std::visit(
        overloaded{
            [&](const LiteralTerminalSymbol& symbol) -> int
            {
                if (index + symbol.size() > src.size())
                {
                    return -1;
                }

                const std::string_view subsrc(src.begin() + index, src.begin() + index + symbol.size());
                const auto matches = subsrc == symbol;

                return matches ? static_cast<int>(subsrc.size()) : -1;
            },
            [&](const ChoiceTerminalSymbol& symbol) -> int
            {
                for (const auto& partial : symbol)
                {
                    if (index + partial.size() >= src.size())
                    {
                        continue;
                    }

                    const std::string_view subsrc(src.begin() + index, src.begin() + index + partial.size());
                    if (const auto matches = subsrc == partial)
                    {
                        return static_cast<int>(subsrc.size());
                    }
                }

                return -1;
            },
            [&](const RangeTerminalSymbol& symbol) -> int
            {
                if (index >= src.size())
                {
                    return -1;
                }

                if (const auto matches = src[index] >= symbol.first[0] && src[index] <= symbol.second[0])
                {
                    return 1;
                }

                return -1;
            },
            [&](const RegexTerminalSymbol& symbol) -> int
            {
                if (index >= src.size())
                {
                    return -1;
                }

                if (std::cmatch match; std::regex_search(src.data() + index, src.data() + src.size(), match, symbol, std::regex_constants::match_continuous))
                {
                    return match[0].length();
                }

                return -1;
            }},
        symbol);
}

std::ostream& operator<<(std::ostream& os, const StringGrammar::TerminalSymbol& symbol)
{
    using namespace StringGrammar;
    std::visit(
        overloaded{
            [&](const LiteralTerminalSymbol& symbol)
            {
                os << '"';
                printUnescaped(symbol, os);
                os << '"';
            },
            [&](const ChoiceTerminalSymbol& symbol)
            {
                os << '(';
                bool first = true;
                for (const auto& partial : symbol)
                {
                    if (!first)
                    {
                        os << " | ";
                    }
                    first = false;

                    os << '"';
                    printUnescaped(partial, os);
                    os << '"';
                }

                os << ')';
            },
            [&](const RangeTerminalSymbol& symbol)
            {
                os << '[';
                printUnescaped(symbol.first, os);
                os << '-';
                printUnescaped(symbol.second, os);
                os << ']';
            },
            [&](const RegexTerminalSymbol& symbol)
            {
                os << '/' << symbol.pattern << '/';
            }},
        symbol);

    return os;
}

} // namespace StringGrammar


template <typename ParserTypes>
struct StringGrammarBuilder
{
    using NT = typename ParserTypes::NonTerminal;
    using LT = typename ParserTypes::Terminal;

    NT startSymbol;
    std::optional<NT> whitespaceSymbol;

    std::vector<Rule<ParserTypes>> rules;
    Semantics<ParserTypes> semantics;

    using Str = StringGrammar::Str;

    StringGrammarBuilder(NT startSymbol, std::optional<NT> whitespaceSymbol = {}) : startSymbol{startSymbol}, whitespaceSymbol{whitespaceSymbol}
    {

    }

    struct StringSymbolBuilder
    {
      protected:
        StringSymbolBuilder(StringGrammar::TerminalSymbol symbol) : symbol{std::move(symbol)}
        {

        }

      public: 
        StringGrammar::TerminalSymbol symbol;
    };

    struct Range : public StringSymbolBuilder
    {
        explicit Range(const Str& start, const Str& end) : StringSymbolBuilder(StringGrammar::RangeTerminalSymbol{start, end})
        {
        }
    };

    struct Choice : public StringSymbolBuilder
    {
        template <typename... Args>
        explicit Choice(Args... args) : StringSymbolBuilder(StringGrammar::ChoiceTerminalSymbol{args...})
        {
        }
    };

    struct Regex : public StringSymbolBuilder
    {
        template <typename... Args>
        explicit Regex(Args... args) : StringSymbolBuilder(StringGrammar::RegexTerminalSymbol{args...})
        {
        }
    };

    struct RuleBuilder
    {
        StringGrammarBuilder& grammarBuilder;

        Rule<ParserTypes> rule;
        Semantics<ParserTypes>::SemanticAction action;

        void addWhitespace(bool checkForDuplicate)
        {
            const auto& whitespace = grammarBuilder.whitespaceSymbol;
            if (!whitespace)
            {
                return;
            }

            if (rule.product == whitespace)
            {
                return;
            }

            if (checkForDuplicate && !rule.symbols.empty())
            {
                if (NT* lastSymbol = std::get_if<NT>(&rule.symbols.back()))
                {
                    if (*lastSymbol == whitespace)
                    {
                        return;
                    }
                }
            }

            rule.add(*whitespace, true);
        }

        RuleBuilder& operator&(const StringSymbolBuilder& symbolBuilder)
        {
            addWhitespace(true);
            rule.add(symbolBuilder.symbol);
            addWhitespace(false);

            return *this;
        }

        RuleBuilder& operator&(const NT& token)
        {
            rule.add(token);
            return *this;
        }

        RuleBuilder& operator&(const LT& token)
        {
            addWhitespace(true);
            rule.add(token);
            addWhitespace(false);

            return *this;
        }

        template <typename T>
        RuleBuilder& operator>>(T&& t)
        {
            return *this & std::forward<T>(t);
        }

        void operator|(Semantics<ParserTypes>::SemanticAction semanticAction)
        {
            action = std::move(semanticAction);
        }

        ~RuleBuilder()
        {
            rule.id = grammarBuilder.rules.size();
            grammarBuilder.semantics.setAction(rule.id, action);
            grammarBuilder.rules.emplace_back(std::move(rule));
        }
    };

    auto operator()(NT token)
    {
        return RuleBuilder{*this, token};
    }

    template <typename... Args>
    static auto make(Args... args)
    {
        return std::array<Rule<ParserTypes>, sizeof...(args)>{args.rule...};
    }

    using Terminal = char;

    Grammar<ParserTypes> getGrammar() const
    {
        return {startSymbol, rules};
    }

    const Semantics<ParserTypes>& getSemantics() const
    {
        return semantics;
    }

    Parser<ParserTypes> makeParser() const
    {
        return 
        {
            Grammar<ParserTypes>{startSymbol, rules},
            &StringGrammar::match,
            semantics,
        };
    }
};

} // namespace larley