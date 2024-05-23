#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>

#include "magic_enum/magic_enum.hpp"
#include "magic_enum/magic_enum_flags.hpp"
#include "magic_enum/magic_enum_iostream.hpp"
using magic_enum::iostream_operators::operator<<; // out-of-the-box ostream operators for enums.

#include "larley/parser-types.hpp"
#include "larley/grammar.hpp"
#include "larley/parser.hpp"
#include "larley/string-grammar.hpp"
#include "larley/tree-builder.hpp"
#include "larley/apply-semantics.hpp"
#include "larley/parsing-error.hpp"

using namespace larley;

//	Sum     -> Sum     [+-] Product | Product
//	Product -> Product [*/] Factor | Factor
//	Factor  -> '(' Sum ')' | Number
//	Number  -> [0-9] Number | [0-9]

enum NonTerminals
{
	Sum,
	Product,
	Factor,
	Number,
	Digit,
	Digits,
	Whitespace
};

void printTree(const auto& tree, const auto& str)
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
			std::cout << magic_enum::enum_name(edge.rule->product) << " ( " << edge.start << ", " << edge.end << ")\n";

			for (int x = 0; x < edge.rule->symbols.size(); x++)
			{
				iter(depth + 1);
			}
		}
		else
		{
            std::cout << '"' << str.substr(edge.start, edge.end - edge.start) << "\" ( " << edge.start << ", " << edge.end << ")\n";
		}
	};

	iter();

	std::cout << std::endl << std::endl;
}

void printError(const auto& parserError, const auto& inputs)
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

    std::size_t maxSymbolLength{};
    for (const auto& rule : inputs.grammar.rules)
    {
        const auto name = magic_enum::enum_name(rule.product);
        maxSymbolLength = std::max(maxSymbolLength, name.size());
    }

	for (const auto& prediction : parserError.predictions)
	{
        std::cout << prediction.terminal << " from:" << std::endl;

        for (const auto itemPtr : prediction.path)
        {
            const auto& item = *itemPtr;
            const auto& rule = item.rule;
            const auto name = magic_enum::enum_name(rule.product);
            std::cout << "    " << std::setw(maxSymbolLength) << name;
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
                    std::cout << magic_enum::enum_name(*NT);
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

	std::size_t maxSymbolLength{};
	for (const auto& rule : grammar.rules)
	{
		const auto name = magic_enum::enum_name(rule.product);
		maxSymbolLength = std::max(maxSymbolLength, name.size());
	}

	for (const auto& rule : grammar.rules)
	{
		const auto name = magic_enum::enum_name(rule.product);
		std::cout << std::setw(maxSymbolLength) << name;
		std::cout << " ->";

		for (const auto& symbol : rule.symbols)
		{
			std::cout << " ";
			if (auto* NT = std::get_if<0>(&symbol))
			{
				std::cout << magic_enum::enum_name(*NT);
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

void printChart(const auto& inputs, const auto& parseResult)
{
	std::cout << "-------- Charts --------" << std::endl;

	std::size_t maxSymbolLength{};
	for (const auto& rule : inputs.grammar.rules)
	{
		const auto name = magic_enum::enum_name(rule.product);
		maxSymbolLength = std::max(maxSymbolLength, name.size());
	}

	const auto& S = parseResult.S;
	for (std::size_t x = 0; x < S.size(); x++)
	{
		std::cout << "Chart: " << x << std::endl;
		const auto& set = S[x];
		for (std::size_t y = 0; y < set.size(); y++)
		{
			const auto& item = set[y];
			const auto& rule = item.rule;
			const auto name = magic_enum::enum_name(rule.product);
			std::cout << std::setw(maxSymbolLength) << name;
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
					std::cout << magic_enum::enum_name(*NT);
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

void testMaths()
{
	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using Range = typename GB::Range;
	using Regex = typename GB::Regex;
	using Choice = typename GB::Choice;

	using Src = PT::Src;

 // clang-format off

	/*
	GB gb{Sum};
	gb(Sum)     >> Sum & Choice{"+", "-"} & Product     | [](const auto& vals) { return std::any_cast<int>(vals[0]) + std::any_cast<int>(vals[2]); };
	gb(Sum)     >> Product;
	gb(Product) >> Product & Choice{"*", "/"} & Factor  | [](const auto& vals) { return std::any_cast<int>(vals[0]) * std::any_cast<int>(vals[2]); };
	gb(Product) >> Factor;
	gb(Factor)  >> "(" & Sum & ")"                      | [](const auto& vals) { return vals[1]; };
	gb(Factor)  >> Digit;
	gb(Digit)   >> Range{"0", "9"}    
	*/

	//GB gb{Sum, Whitespace};
	GB gb{Sum};
	gb(Whitespace);
	gb(Whitespace) >> Regex{"\\s+"};
	gb(Sum)     >> Sum & "+" & Product          | [](const auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
	gb(Sum)     >> Sum & "-" & Product          | [](const auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
	gb(Sum)     >> Product;
	gb(Product) >> Product & "*" & Factor       | [](const auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
	gb(Product) >> Product & "/" & Factor       | [](const auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
	gb(Product) >> Factor;
	gb(Factor)  >> "(" & Sum & ")"              | [](const auto& vals) { return vals[1]; };
	gb(Factor)  >> Digit;
	gb(Digit)   >> Regex{"[0-9]+(\\.[0-9]+)?"}  | [](const auto& vals)
		{ 
			auto src = vals[0].as<Src>();
			float result;
			std::from_chars(src.data(), src.data() + src.size(), result);
			return result;
		};
	
// clang-format on

	//std::string str = " 1 + ( 2  / 3 ) \t* \t\t\t4.5 ";
    std::string str = "1+1(";

	const auto inputs = gb.makeInputs(str);

	printGrammar(inputs.grammar);

	auto parsed = parse(inputs);
	std::cout << "Match found: " << parsed.matchCount << std::endl;
	printChart(inputs, parsed);

	const auto error = makeParseError(inputs, parsed);
    printError(error, inputs);

	auto tree = buildTree(inputs, parsed);
	printTree(tree, str);

	auto result = applySemantics(inputs, tree);
	std::cout << std::any_cast<float>(result) << std::endl;
}

/*
enum IfGrammar
{
	Block,
	If
};

void testPriority()
{
	using GB = StringGrammarBuilder<IfGrammar, StringGrammar::TerminalSymbol>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb1;
	gb1(Block) >> "{}";
	gb1(Block) >> If;
	gb1(If) >> "if" & Block;
	gb1(If) >> "if" & Block & "else" & Block;

	Parser p1{gb1.getGrammar()};

	GB gb2;
	gb2(Block) >> "{}";
	gb2(Block) >> If;
	gb2(If) >> "if" & Block & "else" & Block;
	gb2(If) >> "if" & Block;

	Parser p2{gb2.getGrammar()};

	std::string str = "ifif{}else{}";
	auto span = std::span<char>{str.begin(), str.end()};

	p1.parse(span, StringGrammar::match, Block);
	auto tree1 = p1.buildTree(span, StringGrammar::match, Block);

	p2.parse(span, StringGrammar::match, Block);
	auto tree2 = p2.buildTree(span, StringGrammar::match, Block);

	printTree(tree1, str);
	printTree(tree2, str);
}

enum BlowoutGrammar
{
	A,
};

void testBlowout()
{
	using GB = StringGrammarBuilder<BlowoutGrammar, StringGrammar::TerminalSymbol>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb;
	gb(A) >> A;
	gb(A);

	Parser p{gb.getGrammar()};

	std::string str = "";
	auto span = std::span<char>{str.begin(), str.end()};

	p.parse(span, StringGrammar::match, A);
	auto tree = p.buildTree(span, StringGrammar::match, A);

	printTree(tree, str);
}

void testEmptyRuleGrammar()
{
	enum EmptyGrammar
	{
		A,
		B,
		S,
		T
	};

	using GB = StringGrammarBuilder<EmptyGrammar, StringGrammar::TerminalSymbol>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb;
	gb(A); // Empty
	// gb(A) >> B;
	gb(B) >> A;
	gb(S) >> "S";
	gb(T); // Empty
	gb(T) >> T& A & S & B;

	Parser p{gb.getGrammar()};
	std::string str = "S";
	p.parse(std::span<char>{str.begin(), str.end()}, StringGrammar::match, T);
	printTree(p.buildTree(std::span<char>{str.begin(), str.end()}, StringGrammar::match, T), str);
}
*/

int main()
{
    //SetConsoleCP(65001);
    //SetConsoleOutputCP(65001);

	//testEmptyRuleGrammar();
	//testPriority();
	testMaths();

	try
	{
		//testBlowout();
	}
	catch (...)
	{
	}
}