#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>

#include "../utils.hpp"

#include "larley/parser-types.hpp"
#include "larley/printer.hpp"
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

void testMaths()
{
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
	gb(Sum)     >> Sum & "+" & Product          | [](auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
	gb(Sum)     >> Sum & "-" & Product          | [](auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
	gb(Sum)     >> Product;
	gb(Product) >> Product & "*" & Factor       | [](auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
	gb(Product) >> Product & "/" & Factor       | [](auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
	gb(Product) >> Factor;
	gb(Factor)  >> "(" & Sum & ")"              | [](auto& vals) { return vals[1]; };
	gb(Factor)  >> Digit;
	gb(Digit)   >> Regex{"[0-9]+(\\.[0-9]+)?"}  | [](auto& vals)
		{ 
			auto src = vals[0].src;
			float result;
			std::from_chars(src.data(), src.data() + src.size(), result);
			return result;
		};
	
// clang-format on

	//std::string str = " 1 + ( 2  / 3 ) \t* \t\t\t4.5 ";
    std::string str = "1+1+1+1";

	const auto inputs = gb.makeInputs(str);
    const auto printer = gb.makePrinter(inputs, &enumToString<NonTerminals>);

	printGrammar(printer, inputs.grammar);

	auto parsed = parse(inputs);
	std::cout << "Match found: " << parsed.matchCount << std::endl;
    printChart(printer, inputs, parsed);

	const auto error = makeParseError(inputs, parsed);
    printError(printer, inputs, error);

	auto tree = buildTree(inputs, parsed);
    printTree(printer, inputs, tree);

	auto result = applySemantics(inputs, tree);
	std::cout << std::any_cast<float>(result) << std::endl;
}

void testPriority()
{
    enum IfGrammar
    {
        Block,
        If
    };
	
	using PT = ParserTypes<IfGrammar, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb1(Block);
	gb1(Block) >> "{}";
	gb1(Block) >> If;
	gb1(If) >> "if" & Block;
	gb1(If) >> "if" & Block & "else" & Block;


	GB gb2(Block);
	gb2(Block) >> "{}";
	gb2(Block) >> If;
	gb2(If) >> "if" & Block & "else" & Block;
	gb2(If) >> "if" & Block;

	std::string str = "ifif{}else{}";

	const auto inputs1{gb1.makeInputs(str)};
    const auto inputs2{gb2.makeInputs(str)};

    const auto printer1{gb1.makePrinter(inputs1, enumToString<IfGrammar>)};
    const auto printer2{gb2.makePrinter(inputs2, enumToString<IfGrammar>)};

	const auto parsed1 = parse(inputs1);
    const auto parsed2 = parse(inputs2);
    
	auto tree1 = buildTree(inputs1, parsed1);
    auto tree2 = buildTree(inputs2, parsed2);

	printTree(printer1, inputs1, tree1);
    printTree(printer2, inputs2, tree2);
}

void testBlowout()
{
    enum BlowoutGrammar
    {
        A,
    };
	
	using PT = ParserTypes<BlowoutGrammar, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb(A);
	gb(A) >> A;
	gb(A);

	std::string str = "";
    const auto inputs = gb.makeInputs(str);

	const auto parsed = parse(inputs);
	const auto tree = buildTree(inputs, parsed);

	printTree(gb.makePrinter(inputs, enumToString<BlowoutGrammar>), inputs, tree);
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
	
	using PT = ParserTypes<EmptyGrammar, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using RuleBuilder = typename GB::RuleBuilder;
	using Range = typename GB::Range;
	using Choice = typename GB::Choice;

	GB gb(T);
	gb(A); // Empty
	// gb(A) >> B;
	gb(B) >> A;
	gb(S) >> "S";
	gb(T); // Empty
	gb(T) >> T& A & S & B;

	std::string str = "S";
    const auto inputs = gb.makeInputs(str);

    const auto parsed = parse(inputs);
    const auto tree = buildTree(inputs, parsed);

    printTree(gb.makePrinter(inputs, enumToString<EmptyGrammar>), inputs, tree);
}

int main()
{
	testEmptyRuleGrammar();
	testPriority();
	testMaths();

	try
	{
		testBlowout();
	}
	catch (...)
	{
	}
}