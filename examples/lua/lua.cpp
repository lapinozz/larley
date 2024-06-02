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

void parseLua()
{
    enum NonTerminals
    {
        Ws,
        Chunk,
        Block,
        Stat,
        Stats,
        RetStat,
        VarList,
        ExpList,
        FunctionCall,
        Label,
        Name,
        Exp,
        ElseIfs,
        ElseIf,
        Else,
        Exps,
        NameList,
        FuncName,
        FuncBody,
        DotNames,
        Var,
        PrefixExp,
        Numeral,
        LiteralString,
        FunctionDef,
        TableConstructor,
        BinOp,
        UnOp,
        Args,
        PartList,
        Field,
        FieldList,
        FieldSep,
    };

	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using Range = typename GB::Range;
	using Regex = typename GB::Regex;
	using Choice = typename GB::Choice;

	using Src = PT::Src;

 // clang-format off

	GB gb{Chunk, Ws};

    gb(Ws);
    gb(Ws) >> Regex{"\\s+"};
    gb(Ws) >> Regex{"(\\s*--.*\\n\\s*)+"};

    gb(Name) >> Regex{"[a-zA-Z_][a-zA-Z_0-9]*"};

    gb(LiteralString) >> "\"" & Regex("[^\"]*") & "\"";
    gb(LiteralString) >> "'" & Regex("[^']*") & "'";

    gb(Numeral) >> Regex{"[0-9]+(.[0-9]+)?"};

	gb(Chunk) >> Block;

	gb(Block) >> Stats;
	gb(Block) >> Stats & RetStat;

	gb(Stats);
	gb(Stats) >> Stats & Stat;

	gb(Stat) >> ";";
	gb(Stat) >> VarList & "=" & ExpList;
	gb(Stat) >> FunctionCall;
	gb(Stat) >> Label;
	gb(Stat) >> "break";
	gb(Stat) >> "goto" & Name;
	gb(Stat) >> "do" & Block & "end";
	gb(Stat) >> "while" & Exp & "do" & Block & "end";
	gb(Stat) >> "if" & Exp & "then" & Block & ElseIfs & Else & "end";
	gb(Stat) >> "for" & Name & "=" & Exps & "do" & Block & "end";
	gb(Stat) >> "for" & NameList & "in" & ExpList & "do" & Block & "end";
	gb(Stat) >> "function" & FuncName & FuncBody;
	gb(Stat) >> "local" & "function" & Name & FuncBody;
	gb(Stat) >> "local" & NameList;
	gb(Stat) >> "local" & NameList & "=" & ExpList;

    gb(ElseIfs);
    gb(ElseIfs) >> ElseIf;
    gb(ElseIfs) >> ElseIfs & ElseIf;

    gb(Else);
    gb(Else) >> "else" & Block;

    gb(Exps) >> Exp;
    gb(Exps) >> Exps & "," & Exp;

    gb(RetStat) >> "return";
    gb(RetStat) >> "return" & ExpList;
    gb(RetStat) >> "return" & ";";
    gb(RetStat) >> "return" & ExpList & ";";

    gb(Label) >> "::" & Name & "::";
    gb(Label) >> "::" & Name & "::";

    gb(DotNames);
    gb(DotNames) >> DotNames & "." & Name;

	gb(FuncName) >> Name & DotNames;
	gb(FuncName) >> Name & DotNames & ":" & Name;

    gb(VarList) >> Var;
    gb(VarList) >> VarList & "," & Var;

    gb(Var) >> Name;
    gb(Var) >> PrefixExp & "[" & Exp & "]";
    gb(Var) >> PrefixExp & "." & Name;

    gb(NameList) >> Name;
    gb(NameList) >> NameList & "," & Name;

    gb(ExpList) >> Exp;
    gb(ExpList) >> ExpList & "," & Exp;

    gb(Exp) >> "nil";
    gb(Exp) >> "false";
    gb(Exp) >> "true";
    gb(Exp) >> Numeral;
    gb(Exp) >> LiteralString;
    gb(Exp) >> "...";
    gb(Exp) >> FunctionDef;
    gb(Exp) >> PrefixExp;
    gb(Exp) >> TableConstructor;
    gb(Exp) >> Exp & BinOp & Exp;
    gb(Exp) >> UnOp & Exp;

    gb(PrefixExp) >> Var;
    gb(PrefixExp) >> FunctionCall;
    gb(PrefixExp) >> "(" & Exp & ")";

    gb(FunctionCall) >> PrefixExp & Args;
    gb(FunctionCall) >> PrefixExp & ":" & Name & Args;

    gb(Args) >> "(" & ")";
    gb(Args) >> "(" & ExpList & ")";
    gb(Args) >> TableConstructor;
    gb(Args) >> LiteralString;

    gb(FunctionDef) >> "function" & FuncBody;

    gb(FuncBody) >> "(" & ")" & Block & "end";
    gb(FuncBody) >> "(" & PartList & ")" & Block & "end";

    gb(PartList) >> NameList;
    gb(PartList) >> NameList & "," & "...";
    gb(PartList) >> "...";

    gb(TableConstructor) >> "{" & "}";
    gb(TableConstructor) >> "{" & FieldList & "}";

    gb(FieldList) >> Field;
    gb(FieldList) >> FieldList & FieldSep & Field;
    gb(FieldList) >> FieldList & FieldSep;

    gb(Field) >> "[" & Exp & "]" & "=" & Exp;
    gb(Field) >> Name & "=" & Exp;
    gb(Field) >> Exp;

    gb(FieldSep) >> ",";
    gb(FieldSep) >> ";";

    gb(BinOp) >> "+";
    gb(BinOp) >> "-";
    gb(BinOp) >> "*";
    gb(BinOp) >> "/";
    gb(BinOp) >> "//";
    gb(BinOp) >> "^";
    gb(BinOp) >> "%";
    gb(BinOp) >> "&";
    gb(BinOp) >> "~";
    gb(BinOp) >> "|";
    gb(BinOp) >> ">>";
    gb(BinOp) >> "<<";
    gb(BinOp) >> "..";
    gb(BinOp) >> "<";
    gb(BinOp) >> "<=";
    gb(BinOp) >> ">";
    gb(BinOp) >> ">=";
    gb(BinOp) >> "==";
    gb(BinOp) >> "~=";
    gb(BinOp) >> "and";
    gb(BinOp) >> "or";

    gb(UnOp) >> "-";
    gb(UnOp) >> "not";
    gb(UnOp) >> "#";
    gb(UnOp) >> "~";
	
// clang-format on

    std::ifstream file(std::filesystem::current_path() / "test.lua");
    std::string str(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>{});

	auto parser = gb.makeParser();
    str = str;
    auto start = std::chrono::high_resolution_clock::now();

    parser.parse(str);

    auto end = std::chrono::high_resolution_clock::now();

    auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    std::cout << elapsed << "ms" << std::endl;
    std::cout << (1000 / elapsed) * str.size() / 1000 / 1000 << "Mb/s" << std::endl;
    std::cout << str.size() << "B" << std::endl;

    if (parser.error)
    {
        parser.printError();
    }
    else
    {
        //parser.printTree();
    }
}

int main()
{
    parseLua();
}