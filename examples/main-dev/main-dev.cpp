#include <algorithm>
#include <any>
#include <array>
#include <iomanip>
#include <iostream>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

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
    Digits
};

void printTree(const auto& tree, const auto& str)
{
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
            std::cout << str.substr(edge.start, edge.end - edge.start) << " ( " << edge.start << ", " << edge.end << ")\n";
        }
    };

    iter();
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

    GB gb{Sum};
    gb(Sum)     >> Sum & "+" & Product          | [](const auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
    gb(Sum)     >> Sum & "-" & Product          | [](const auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
    gb(Sum)     >> Product;
    gb(Product) >> Product & "*" & Factor       | [](const auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
    gb(Product) >> Product & "/" & Factor       | [](const auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
    gb(Product) >> Factor;
    gb(Factor)  >> "(" & Sum & ")"              | [](const auto& vals) { return vals[1]; };
    gb(Factor)  >> Digit;
    gb(Digit)   >> Regex{"[0-9]+(\\.[0-9]+)?"}   | [](const auto& vals)
    { 
        auto src = vals[0].as<Src>();
        float result;
        std::from_chars(src.data(), src.data() + src.size(), result);
        return result;
    };
    
// clang-format on

    std::string str = "1+(2/3)*4.5";

    const auto inputs = gb.makeInputs(str);

    auto parsed = parse(inputs);
    auto tree = buildTree(inputs, parsed);
    auto result = applySemantics(inputs, tree);

    printTree(tree, str);

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