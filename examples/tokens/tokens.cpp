#include <any>
#include <string>
#include <span>
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

struct Token
{
    enum class Type
    {
        Add,
        Sub,
        Div,
        Mul,
        ParenOpen,
        ParenClose,
        Number
    };

    Type type;
    std::string value;
};

auto makeParser()
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

	using PT = ParserTypes<NonTerminals, Token::Type, std::span<Token>>;
	using GB = StringGrammarBuilder<PT>;

	using Src = PT::Src;

 // clang-format off
    
	GB gb{Sum};
	gb(Sum)     >> Sum & Token::Type::Add & Product          | [](auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
	gb(Sum)     >> Sum & Token::Type::Sub & Product          | [](auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
	gb(Sum)     >> Product;
	gb(Product) >> Product & Token::Type::Mul & Factor       | [](auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
	gb(Product) >> Product & Token::Type::Div & Factor       | [](auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
	gb(Product) >> Factor;
	gb(Factor)  >> Token::Type::ParenOpen & Sum & Token::Type::ParenClose   | [](auto& vals) { return vals[1]; };
	gb(Factor)  >> Digit;
	gb(Digit)   >> Token::Type::Number  | [](auto& vals)
		{ 
			auto src = vals[0].src;
			float result;
            auto& value = src[0].value;
			std::from_chars(value.data(), value.data() + value.size(), result);
			return result;
		};
	
// clang-format on

    const auto matcher = [](auto src, std::size_t index, auto& type) -> int 
    {
        if (index >= src.size())
        {
            return -1;
        }

        return src[index].type == type ? 1 : -1;
    };

    return Parser<PT>{
        Grammar<PT>{gb.startSymbol, gb.rules},
        matcher,
        gb.semantics,
    };
}

int main()
{
    auto parser = makeParser();

    std::vector<Token> tokens {
        {Token::Type::Number, "1"},
        {Token::Type::Add},
        {Token::Type::Number, "2"},
        {Token::Type::Mul},
        {Token::Type::Number, "3"},
        {Token::Type::Div},
        {Token::Type::Number, "4"},
    };

    if (auto value = parser.parse(tokens); value.has_value())
    {
        std::cout << value.as<float>() << std::endl;
    }
}