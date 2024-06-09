#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>
#include <chrono>
#include <fstream>
#include <streambuf>
#include <filesystem>

#include "../utils.hpp"

#include "larley/string-grammar.hpp"

using namespace larley;

using GenNT = std::string;

using GenPT = ParserTypes<GenNT, StringGrammar::TerminalSymbol, std::string_view>;
using GenGB = StringGrammarBuilder<GenPT>;
using GenRule = larley::Rule<GenPT>;
using GenSymbol = GenRule::Symbol;

auto makeParser()
{
    enum NonTerminals
    {
        Ws,
        Identifier,
        Range,
        Choice,
        Literal,
        Rules,
        Rule,
        Symbols,
        Symbol,
        Definitions,
        Grammar
    };

	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol, std::string_view>;
	using GB = StringGrammarBuilder<PT>;

	using Src = PT::Src;

 // clang-format off

	GB gb{Grammar, Ws};

    gb(Ws);
    gb(Ws) >> GB::Regex{"\\s+"};

    gb(Identifier) >> GB::Regex{"[a-zA-Z_][a-zA-Z_0-9]*"} | [](auto& vals)
	{ 
        return std::string{vals[0].src};
	};

    gb(Range) >> GB::Regex{"\\[.\\-.\\]"} | [](auto& vals) -> GenSymbol
	{ 
        return typename StringGrammar::RangeTerminalSymbol{std::string{vals[0].src[1]}, std::string{vals[0].src[3]}};
	};

    gb(Choice) >> GB::Regex{"\\[[^\\]]+\\]"} | [](auto& vals) -> GenSymbol
	{ 
        StringGrammar::ChoiceTerminalSymbol choices;
        const auto& src = vals[0].src;
        for (const auto c : src.substr(1, src.size() - 2))
        {
            choices.push_back(std::string{c});
        }

        return choices;
	};    

    gb(Literal) >> GB::Regex("\\\"[^\"]*\\\"") | [](auto& vals) -> GenSymbol
	{ 
        const auto& src = vals[0].src;
		return std::string{src.data() + 1, src.data() + src.size() - 1};
	};

    gb(Rule) >> Identifier & "->" & Definitions | [](auto& vals)
	{ 
        auto id = vals[0].as<GenNT>();
        std::vector<GenRule> rules;
        auto& definitions = vals[2].as<std::vector<std::vector<GenSymbol>>>();
        for (auto& definition : definitions)
        {
            rules.emplace_back(id, std::move(definition));
        }
        return rules;
	};

    gb(Rules) >> Rule;
    gb(Rules) >> Rules & Rule | [](auto& vals)
	{ 
        auto& v1 = vals[0].as<std::vector<GenRule>>();
        auto& v2 = vals[1].as<std::vector<GenRule>>();

        v1.insert(v1.end(), v2.begin(), v2.end());

        return std::move(v1);
	};
    
    gb(Definitions) >> Symbols | [](auto& vals)
	{ 
        std::vector<std::vector<GenSymbol>> v;
        v.push_back(vals[0].as<std::vector<GenSymbol>>());
        return std::move(v);
	};
    gb(Definitions) >> Definitions & "|" & Symbols | [](auto& vals)
	{ 
        auto& v = vals[0].as<std::vector<std::vector<GenSymbol>>>();
        v.push_back(vals[2].as<std::vector<GenSymbol>>());
        return std::move(v);
	};

    gb(Symbols) >> Symbol | [](auto& vals)
	{ 
        std::vector<GenSymbol> v;
        v.push_back(vals[0].as<GenSymbol>());
        return std::move(v);
	};
    gb(Symbols) >> Symbols & Symbol | [](auto& vals)
	{ 
        auto& v = vals[0].as<std::vector<GenSymbol>>();
        v.push_back(vals[1].as<GenSymbol>());
        return std::move(v);
	};

    gb(Symbol) >> Range;
    gb(Symbol) >> Choice;
    gb(Symbol) >> Literal;
    gb(Symbol) >> Identifier | [](auto& vals) -> GenSymbol { return vals[0].as<GenNT>(); };

    gb(Grammar) >> Rules;
	
// clang-format on

	return gb.makeParser();
}

auto makeMetaParser(auto& parser, std::string_view grammar)
{
    if (auto value = parser.parse(grammar); value.has_value())
    {
        auto& rules = value.as<std::vector<GenRule>>();

        Semantics<GenPT> semantics;
        semantics.actions.resize(rules.size());

        return Parser<GenPT>{
            {rules[0].product, rules},
            StringGrammar::match,
            semantics
        };
    }
    else
    {
        parser.printError();

        assert(false);
    }
}


int main()
{
    auto parser = makeParser();

    std::string grammar = 
R"==(
    Sum     -> Sum     [+-] Product | Product
	Product -> Product [*/] Factor | Factor
	Factor  -> "(" Sum ")" | Number
	Number  -> [0-9] Number | [0-9]
)==";

    auto metaParser = makeMetaParser(parser, grammar);

    std::string input = "1+2*3/4";
    metaParser.parse(input);
    metaParser.printTree();
}