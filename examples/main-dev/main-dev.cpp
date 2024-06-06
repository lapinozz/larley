#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>
#include <unordered_map>

#include "../utils.hpp"

#include "larley/string-grammar.hpp"

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

	auto parser = gb.makeParser();

	parser.printGrammar();

	if (auto result = parser.parse(str); result.has_value())
    {
        parser.printChart();
        parser.printTree();

        std::cout << std::any_cast<float>(result) << std::endl;
	}
	else
    {
        parser.printChart();
        parser.printError();
	}
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

	auto parser1 = gb1.makeParser();
    auto parser2 = gb2.makeParser();

	parser1.parse(str);
    parser2.parse(str);

    parser1.printTree();
    parser2.printTree();
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
    auto parser = gb.makeParser();
    parser.parse(str);
    parser.printTree();
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
    auto parser = gb.makeParser();
    parser.parse(str);
    parser.printTree();
}

void testCtx()
{
    enum CtxGrammar
    {
        Item,
        List,
    };

	using Ctx = std::unordered_map<std::string_view, int>;

    using PT = ParserTypes<CtxGrammar, StringGrammar::TerminalSymbol, std::string_view, Ctx>;

    using GB = StringGrammarBuilder<PT>;

    using RuleBuilder = typename GB::RuleBuilder;
    using Range = typename GB::Range;
    using Choice = typename GB::Choice;
    using Regex = typename GB::Regex;

    GB gb(List);
    gb(List) >> Item;
    gb(List) >> Item & "," & List;
    gb(Item) >> Regex{"item[0-9]+"} | [](auto& values, Ctx* ctx) {(*ctx)[values[0].src]++; };

	Ctx ctx;
    std::string str = "item0,item1,item0,item45,item0,item67,item45";

    auto parser = gb.makeParser();
    parser.parse(str, &ctx);
    parser.printTree();

	for (const auto& [item, count] : ctx)
	{
        std::cout << item << ": " << count << std::endl;
	}
}

int main()
{ 
	testMaths();
	testEmptyRuleGrammar();
	testPriority();

	try
	{
		testBlowout();
	}
	catch (...)
	{
	}

	testCtx();
}