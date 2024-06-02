#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>

#include "../utils.hpp"

#include "larley/string-grammar.hpp"
#include "json.hpp"

using namespace larley;

void parseJson()
{
    enum NonTerminals
    {
        Json,
        Element,
        Elements,
        Sign,
        Digit,
        Digits,
        Onenine,
        Fraction,
        Ws,
        Integer,
        Member,
        Members,
        Value,
        Array,
        Object,
        String,
        Character,
        Characters,
        Escape,
        Hex,
        Number,
        Exponent,
    };

	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using Range = typename GB::Range;
	using Regex = typename GB::Regex;
	using Choice = typename GB::Choice;

	using Src = PT::Src;

    using SemanticValue = Semantics<PT>::SemanticValue;

    using ObjectMember = Json::Object::value_type;

 // clang-format off

	GB gb{Json};

	gb(Json) >> Element;

	gb(Value) >> Object     | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(vals[0].as<Json::Object>())};};
	gb(Value) >> Array      | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(vals[0].as<Json::Array>())};};
	gb(Value) >> String     | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(vals[0].as<std::string>())};};
	gb(Value) >> Number     | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(vals[0].as<float>())};};
	gb(Value) >> "true"     | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(true)};};
	gb(Value) >> "false"    | [](auto& vals) { return Json::Value{std::make_shared<Json::Data>(false)};};
	gb(Value) >> "null"     | [](auto& vals) { return Json::Value{};};

	gb(Object) >> "{" & Ws & "}"        | [](auto& vals) { return Json::Object{};};
	gb(Object) >> "{" & Members & "}"   | [](auto& vals) { return vals[1];}; 

	gb(Members) >> Member                   | [](auto& vals) { return Json::Object{std::move(vals[0].as<ObjectMember>())}; };
	gb(Members) >> Members & "," & Member   | [](auto& vals)
    { 
        vals[0].as<Json::Object>().insert(std::move(vals[2].as<ObjectMember>()));
        return vals[0];
    };

	gb(Member) >> Ws & String & Ws & ":" & Element | [](auto& vals)
    { 
        return ObjectMember{vals[1].as<std::string>(), vals[4].as<Json::Value>()};
    };

	gb(Array) >> "[" & Ws & "]"         | [](auto& vals) { return Json::Array{};};
	gb(Array) >> "[" & Elements & "]"   | [](auto& vals) { return vals[1];};

	gb(Elements) >> Element                     | [](auto& vals) { return Json::Array{vals[0].as<Json::Value>()};};
	gb(Elements) >> Elements & "," & Element    | [](auto& vals)
    { 
        vals[0].as<Json::Array>().push_back(std::move(vals[2].as<Json::Value>()));
        return vals[0];
    };

	gb(Element) >> Ws & Value & Ws | [](auto& vals) { return vals[1];};

	gb(String) >> "\"" & Characters & "\"" | [](auto& vals) { return std::string(vals[1].src.begin(), vals[1].src.end());};

    gb(Characters);
    gb(Characters) >> Character & Characters;

    gb(Character) >> Regex("[^\"\\\\]");
    gb(Character) >> "\\" & Escape;

    gb(Escape) >> "\"";
    gb(Escape) >> "\\";
    gb(Escape) >> "/";
    gb(Escape) >> "b";
    gb(Escape) >> "f";
    gb(Escape) >> "n";
    gb(Escape) >> "r";
    gb(Escape) >> "t";
    gb(Escape) >> "u" & Hex & Hex & Hex & Hex;
    
    gb(Hex) >> Digit;
    gb(Hex) >> Range("A", "F");
    gb(Hex) >> Range("a", "f");

    gb(Number) >> Integer & Fraction & Exponent  | [](const auto& vals)
		{ 
			auto src = vals[0].src;
			float result;
			std::from_chars(vals[0].src.data(), vals[2].src.data() + vals[2].src.size(), result);
			return result;
		};

    gb(Integer) >> Digit;
    gb(Integer) >> Onenine & Digits;
    gb(Integer) >> "-" & Digit;
    gb(Integer) >> "-" & Onenine & Digits;

    gb(Digits) >> Digit;
    gb(Digits) >> Digit & Digits;

    gb(Digit) >> "0";
    gb(Digit) >> Onenine;

    gb(Onenine) >> Range("1", "9");

    gb(Fraction);
    gb(Fraction) >> "." & Digits;

    gb(Exponent);
    gb(Exponent) >> "E" & Sign & Digits;
    gb(Exponent) >> "e" & Sign & Digits;

    gb(Sign);
    gb(Sign) >> "+";
    gb(Sign) >> "-";

    gb(Ws);
    gb(Ws) >> " " & Ws;
    gb(Ws) >> "\r" & Ws;
    gb(Ws) >> "\n" & Ws;
    gb(Ws) >> "\t" & Ws;
	
// clang-format on

	//std::string str = " 1 + ( 2  / 3 ) \t* \t\t\t4.5 ";
    //std::string str = R"({" name ":" John ", " age ":30, " car ":null})";
    std::string str = R"(
{
  "string": "Hello, world!",
  "number": 42,
  "float": 3.14,
  "boolean": true,
  "nullValue": null,
  "array": [
    "item1",
    2,
    3.0,
    false,
    {
      "nestedObject": "value"
    }
  ],
  "object": {
    "key1": "value1",
    "key2": 2,
    "key3": {
      "subKey1": "subValue1",
      "subKey2": [1, 2, 3]
    }
  },
  "nestedArray": [
    [
      "nested1",
      "nested2",
      {
        "deepKey": "deepValue"
      }
    ],
    [
      "anotherNested1",
      100,
      null
    ]
  ],
  "complexObject": {
    "details": {
      "name": "John Doe",
      "age": 30,
      "address": {
        "street": "1234 Main St",
        "city": "Anytown",
        "zip": "12345"
      },
      "contacts": [
        {
          "type": "email",
          "value": "john.doe@example.com"
        },
        {
          "type": "phone",
          "value": "555-1234"
        }
      ]
    },
    "preferences": {
      "notifications": true,
      "newsletter": false,
      "theme": "dark"
    }
  },
  "specialCharacters": "!@#$%^&*()_+{}:\"<>?[];',./`~"
}
)";

	auto parser = gb.makeParser();

    if (auto result = parser.parse(str); result.has_value())
    {
        Json::print(std::cout, std::any_cast<Json::Value>(*parser.result), 0); 
    }
    else
    {
        parser.printError();
    }
}

int main()
{
    parseJson();
}