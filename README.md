<a name="readme-top"></a>
</br>
<h1 align="center">[L]arley</h1>
<h6 align="center">LApinozz eaRLEY parser</h6>
</br>

## About The Project

Larley is a generic parser that can parse any CFG (context free grammar) with a focus on flexibility. </br>
It's written in C++ and implements the Earley parser algorithm, with big thanks to [Loup Villant for their amazing tutorial](https://loup-vaillant.fr/tutorials/earley-parsing/)

```cpp
#include "larley/string-grammar.hpp"

using namespace larley;

int main()
{
  enum NonTerminals { Sum, Product, Factor};
  
  using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;
  using GB = StringGrammarBuilder<PT>;
  using Range = GB::Range;
  using Choice = GB::Choice;
  
  GB gb{Sum};
  gb(Sum)     >> Product;
  gb(Sum)     >> Sum & "+" & Product      | [](auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
  gb(Sum)     >> Sum & "-" & Product      | [](auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
  gb(Product) >> Factor;
  gb(Product) >> Product & "*" & Factor   | [](auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
  gb(Product) >> Product & "/" & Factor   | [](auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
  gb(Factor)  >> "(" & Sum & ")"          | [](auto& vals) { return vals[1]; };
  gb(Factor)  >> Range{"0", "9"}          | [](auto& vals) -> float { return vals[0].src[0] - '0'; };
  
  auto parser = gb.makeParser();
  
  std::string str = "(1+2)/3*4";
  std::cout << parser.parse(str).as<float>() << std::endl; // outputs 4
}
```

## Getting started

We start with our list of Non-Terminals.
```cpp
enum NonTerminals { Sum, Product, Factor };
```

Then we define the "parser types", a holder for the types used through the parser
1. The first type is our Non-Terminal, it's mostly meant to be an enum or some integer type
2. The second type is for our Terminal symbol, it can be a more complex object<br/>the parser comes with a base type StringGrammar::TerminalSymbol
3. The third type is for the source you're parsing<br/>it should either be std::string_view or some std::span, if you're parsing the output of a tokenizer for example<br/>it defaults to std::string_view
4. The fourth type is an optional Context type, more on that later.
```cpp
using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;
```

The StringGrammarBuilder is a helper class that provides an easy way to define a grammar.
```cpp
using GB = StringGrammarBuilder<PT>;
using Range = GB::Range;
using Choice = GB::Choice;
```

The next step is to define our grammar.<br/>
The string grammar builder's constructor takes a non-terminal which will be the root of the grammar, where the parsing starts<br/>
Range is a terminal that will match any char between its two element (inclusively)<br/>
Choice will match any char in the list<br/>
And obviously a string will simply match itself in the input
```cpp
GB gb{Sum};
gb(Sum)     >> Product;
gb(Sum)     >> Sum & Choice{"+-"} & Product;

gb(Product) >> Factor;
gb(Product) >> Product & Choice{"*/"} & Factor;

gb(Factor)  >> "(" & Sum & ")";
gb(Factor)  >> Range{"0", "9"};
```

#### Adding semantics

A semantic action is a function that takes a list of semantic values and returns a new semantic value.<br/>
The function will receive a std::vector<SemanticValue>, with one value per symbol in the rule.<br/>
A semantic value is a std::any with an additional view (string_view/span) to the part of the input that the symbol it originates from matched.<br/>

If a rule doesn't have a semantic action it will simply return the semantic value of its first symbol or an empty value if it has none

Note that the Sum and Product rules were split here to make the semantic actions easier to write.

```cpp
GB gb{Sum};
gb(Sum)     >> Product;
gb(Sum)     >> Sum & "+" & Product      | [](auto& vals) { return vals[0].as<float>() + vals[2].as<float>(); };
gb(Sum)     >> Sum & "-" & Product      | [](auto& vals) { return vals[0].as<float>() - vals[2].as<float>(); };
gb(Product) >> Factor;
gb(Product) >> Product & "*" & Factor   | [](auto& vals) { return vals[0].as<float>() * vals[2].as<float>(); };
gb(Product) >> Product & "/" & Factor   | [](auto& vals) { return vals[0].as<float>() / vals[2].as<float>(); };
gb(Factor)  >> "(" & Sum & ")"          | [](auto& vals) { return vals[1]; };
gb(Factor)  >> Range{"0", "9"}          | [](auto& vals) -> float { return vals[0].src[0] - '0'; };
```

The last part is to generate a parser from the grammar builder and use it to parse some input.
The parse function returns the semantic value the parse true was reduced to.

```cpp
auto parser = gb.makeParser();

std::string str = "(1+2)/3*4";
std::cout << parser.parse(str).as<float>() << std::endl; // outputs 4
```

## Improving

We can improve the previous grammar to suport multi-digit numbers with a regex terminal, provided by the default string grammar helpers.

```cpp
gb(Factor) >> Regex{"[0-9]+(\\.[0-9]+)?"} | [](auto& vals)
{ 
  auto src = vals[0].src;
  float result;
  std::from_chars(src.data(), src.data() + src.size(), result);
  return result;
};
```

To support whitespaces/separator we can pass our whitespace non-terminal as the second parameter to the StringGrammarBuilder's constructor.
This will automatically add the whitespace non-terminal around any terminal we add to the grammar, avoiding duplicates where possible

Also notice that the first Whitespace rule was left empty to indicate it is a nullable symbol

```cpp
GB gb{Sum, Whitespace};
gb(Whitespace); // Empty
gb(Whitespace) >> Regex{"\\s+"};
```

## Printing

The parser comes with some printers that can be quite helpful in debugging

<details>
  <summary>parser.printGrammar();</summary>
  
    -------- Grammar --------
        Sum -> Product
        Sum -> Sum "+" Product
        Sum -> Sum "-" Product
    Product -> Factor
    Product -> Product "*" Factor
    Product -> Product "/" Factor
     Factor -> "(" Sum ")"
     Factor -> [0-9]
</details>

<details>
  <summary>parser.printChart();</summary>
  
      -------- Charts --------
      Chart: 0
          Sum ->•Product (0)
          Sum ->•Sum "+" Product (0)
          Sum ->•Sum "-" Product (0)
      Product ->•Factor (0)
      Product ->•Product "*" Factor (0)
      Product ->•Product "/" Factor (0)
       Factor ->•"(" Sum ")" (0)
       Factor ->•[0-9] (0)
      
      Chart: 1
       Factor -> "("•Sum ")" (0)
          Sum ->•Product (1)
          Sum ->•Sum "+" Product (1)
          Sum ->•Sum "-" Product (1)
      Product ->•Factor (1)
      Product ->•Product "*" Factor (1)
      Product ->•Product "/" Factor (1)
       Factor ->•"(" Sum ")" (1)
       Factor ->•[0-9] (1)`

       ...
      
      Chart: 9
       Factor -> [0-9]• (8)
      Product -> Product "*" Factor• (0)
          Sum -> Product• (0)
      Product -> Product•"*" Factor (0)
      Product -> Product•"/" Factor (0)
          Sum -> Sum•"+" Product (0)
          Sum -> Sum•"-" Product (0)
</details>

<details>
  <summary>parser.printTree();</summary>
  
    -------- ParseTree --------
    Sum ( 0, 9)
      Product ( 0, 9)
        Product ( 0, 7)
          Product ( 0, 5)
            Factor ( 0, 5)
              "(" ( 0, 1)
              Sum ( 1, 4)
                Sum ( 1, 2)
                  Product ( 1, 2)
                    Factor ( 1, 2)
                      "1" ( 1, 2)
                "+" ( 2, 3)
                Product ( 3, 4)
                  Factor ( 3, 4)
                    "2" ( 3, 4)
              ")" ( 4, 5)
          "/" ( 5, 6)
          Factor ( 6, 7)
            "3" ( 6, 7)
        "*" ( 7, 8)
        Factor ( 8, 9)
          "4" ( 8, 9)
</details>

## Errors

The parser will automatically populate the error field when the parsing fails.
The error contains the position where the input stopped making sense, either because no active rule matched or because of an unexpected end of the input.
It also contains a list of the terminals it was expecting at that position and the rule sequence that led to it.

It also comes with a helper to print the error

```cpp
std::string str = "1+";
if (auto result = parser.parse(str); result.has_value())
{
  std::cout << result.as<float>() << std::endl;
}
else
{
  parser.printError();
}
```

<details>
  <summary>parser.printError();</summary>
  
      -------- Error --------
      Unexpected end of input
      Line 0 column 2
      1+
        ^
      
      Expected one of the following:
      "(" from:
           Factor ->•"(" Sum ")" (2)
          Product ->•Factor (2)
              Sum -> Sum "+"•Product (0)
      
      [0-9] from:
           Factor ->•[0-9] (2)
          Product ->•Factor (2)
              Sum -> Sum "+"•Product (0)
</details>

## Context

You can define a Context type, an arbritary object that can be used to read or write additional data during the semantic actions.
It's optionally passed as the second argument to the semantic actions.

```cpp
enum CtxGrammar
{
  Item,
  List,
};

using Ctx = std::unordered_map<std::string_view, int>;

using PT = ParserTypes<CtxGrammar, StringGrammar::TerminalSymbol, std::string_view, Ctx>;

using GB = StringGrammarBuilder<PT>;

using RuleBuilder = typename GB::RuleBuilder;
using Regex = typename GB::Regex;

GB gb(List);
gb(List) >> Item;
gb(List) >> Item & "," & List;
gb(Item) >> Regex{"item[0-9]+"} | [](auto& values, Ctx* ctx) {(*ctx)[values[0].src]++; };

Ctx ctx;
std::string str = "item0,item1,item0,item45,item0,item67,item45";

auto parser = gb.makeParser();
parser.parse(str, &ctx);

for (const auto& [item, count] : ctx)
{
  std::cout << item << ": " << count << std::endl;
}

/* Outputs:
  item0: 3
  item67: 1
  item1: 1
  item45: 2
*/
```

## Ambiguities

If the parse is ambiguous the tree is resolved based on two rules
* First it will prioritize rules based on their order in the grammar, first in the grammar having the higher priority
* Second, if the same rule matches in multiple ways, it will select the longuest match

Consider the classic "dangling else" ambiguity, we can select which if the else belongs to by changing the order of the rules

```cpp
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
```

```
-------- ParseTree -------- (gb1)
Block ( 0, 12)
  If ( 0, 12)
    "if" ( 0, 2)
    Block ( 2, 12)
      If ( 2, 12)
        "if" ( 2, 4)
        Block ( 4, 6)
          "{}" ( 4, 6)
        "else" ( 6, 10)
        Block ( 10, 12)
          "{}" ( 10, 12)

-------- ParseTree -------- (gb2)
Block ( 0, 12)
  If ( 0, 12)
    "if" ( 0, 2)
    Block ( 2, 6)
      If ( 2, 6)
        "if" ( 2, 4)
        Block ( 4, 6)
          "{}" ( 4, 6)
    "else" ( 6, 10)
    Block ( 10, 12)
      "{}" ( 10, 12)
```

# Examples

In the examples folder you can finda series of simple implementations showcasing the usage of Larley, notably:

### JSON
  A simple but functional JSON parser.

### Prox
  A simple programing lanuage loosely based on Lox from [Crafting Interpreters](https://craftinginterpreters.com/)

```
fun fibonacci(num)
{
  if (num <= 1) return 1;

  return fibonacci(num - 1) + fibonacci(num - 2);
}

fun main()
{
    println("hello world" || print("notshown"));

    var i = 3;
    i = (i * i + 2 / i) - -1.5;
    if(i < 3)
    {
        println(i, "is less than 3");
    }
    else
    {
        println(i, "is more than 3");
    }

    if(true == !false && !false != !!false)
    {
        println(fibonacci(4));
    }

    var n = 4;
    while(n = n - 1)
    {
        print(n);
    }
}

main();
```

### meta-parser
It defines a parser for a grammar definition for inputs that look like
```
Sum     -> Sum     [+-] Product | Product
Product -> Product [*/] Factor | Factor
Factor  -> '(' Sum ')' | Number
Number  -> [0-9] Number | [0-9]
```

It then makes a new parser from that grammar, which can then parse inputs like `1+2*(3/4)`.
It's meant as an example of the flexibility and dynamic nature of the library

### tokens
An example of how you could use the library with the output of a lexer, using a std::span of a custom token type instead of a std::string_view<br/>
The parser is not limited to strings

### Lua
Defines the whole grammar for the Lua programming language
But it only does the raw parsing into a CST, not an actual AST and no execution/evaluation
