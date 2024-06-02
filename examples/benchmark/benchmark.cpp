#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>
#include <array>
#include <cstdlib>
#include <chrono>

#include "../utils.hpp"

#include "larley/string-grammar.hpp"

using namespace larley;

void parseJson()
{
    enum NonTerminals
    {
        Array,
        Digits,
        Digit
    };

	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using Range = typename GB::Range;
	using Regex = typename GB::Regex;
	using Choice = typename GB::Choice;

	using Src = PT::Src;

 // clang-format off

	GB g1{Array};

	g1(Array) >> Digits;
	g1(Array) >> Array & "," & Digits;

	g1(Digits) >> Digit;
	g1(Digits) >> Digits & Digit;

	g1(Digit) >> Range("0", "9");

	GB g2{Array};

	g2(Array) >> Digits;
	g2(Array) >> Array & "," & Digits;

	g2(Digits) >> Regex("[0-9]+");

	GB g3{Array};

	g3(Array) >> Digits;
	g3(Array) >> Digits & "," & Array;

	g3(Digits) >> Regex("[0-9]+");
	
// clang-format on

	std::array parsers{g1.makeParser(), g2.makeParser(), g3.makeParser()};

	const auto makeShortList = [](int length = 0)
	{
        std::string str;
		for (int x = 0; x < length; x++)
		{
            str += ('0' + (rand() % 10));
            if (x % 25 == 0)
                str += ',';
		}
            str += '0';
        return str;
	};

	const auto runTest = [&](std::string_view name, std::string_view str, int iterations)
	{
        std::array<double, 3> avgs{0,0};
        std::array<double, 3> mins{std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};
        std::array<double, 3> maxs{0, 0};

		for (int it = 0; it < iterations; it++)
        {
            for (int p = 0; p < parsers.size(); p++)
            {
                auto start = std::chrono::high_resolution_clock::now();

                parsers[p].parse(str);

                auto end = std::chrono::high_resolution_clock::now();

                auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
                avgs[p] += elapsed / iterations;
                mins[p] = std::min(mins[p], elapsed);
                maxs[p] = std::max(maxs[p], elapsed);
            }
		}

		std::cout << name << std::endl;
        std::cout << str << std::endl;
        std::cout << str.size() << std::endl;
        
        for (int p = 0; p < parsers.size(); p++)
        {
            std::cout << p << ' ' << avgs[p] << ' ' << mins[p] << ' ' << maxs[p] << ' ' << std::fixed << ((1000 / avgs[p]) * str.size()) / 1000 / 1000 << "Mb/s\n";

        }
        std::cout << '\n';
    };

	runTest("test", makeShortList(1000), 250);
}

int main()
{
    parseJson();
}