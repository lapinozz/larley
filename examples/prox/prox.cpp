#include <any>
#include <string>
#include <iostream>
#include <print>
#include <iomanip>
#include <charconv>

#include "../utils.hpp"

#include "larley/string-grammar.hpp"
#include "prox.hpp"

using namespace larley;

auto makeParser()
{
    enum NonTerminals
    {
        Program,
        Declarations,
        Declaration,
        FunDecl,
        VarDecl,
        Statement,
        ExprStmt,
        IfStmt,
        ReturnStmt,
        WhileStmt,
        Block,
        Expression,
        Assignment,
        LogicOr,
        LogicAnd,
        Equality,
        Comparison,
        Term,
        Factor,
        Unary,
        Call,
        Primary,
        Function,
        Parameters,
        Arguments,
        Whitespace,
        Number,
        String,
        Identifier
    };

	using PT = ParserTypes<NonTerminals, StringGrammar::TerminalSymbol>;

	using GB = StringGrammarBuilder<PT>;

	using Range = typename GB::Range;
	using Regex = typename GB::Regex;
	using Choice = typename GB::Choice;

	using Src = PT::Src;

    using SemanticValue = Semantics<PT>::SemanticValue;

    const auto makeBinaryAction = [](Prox::BinaryExpr::Op op)
    {
        return [=](auto& vals)
        {
            return std::make_shared<Prox::Expr>(Prox::BinaryExpr{vals[0].as<Prox::ExprPtr>(), op, vals[2].as<Prox::ExprPtr>()});
        };
    };

    const auto makeArrayStart = []<typename T>()
    {
        return [](auto& vals)
        {
            if (vals.empty())
            {
                return std::vector<T>{};
            }
            else
            {
                return std::vector<T>{vals[0].as<T>()};
            }
        };
    };

    const auto makeArrayAppend = []<typename T>()
    {
        return [](auto& vals)
        {
            auto& vec = vals[0].as<std::vector<T>>();
            vec.emplace_back(std::move(vals.back().as<T>()));
            return std::move(vec);
        };
    };

 // clang-format off

	GB gb{Program, Whitespace};

	gb(Program) >> Declarations;

	gb(Declarations)                               | makeArrayStart.template operator()<Prox::StmtPtr>();
	gb(Declarations) >> Declarations & Declaration | makeArrayAppend.template operator()<Prox::StmtPtr>();

	gb(Declaration) >> FunDecl;
	gb(Declaration) >> VarDecl;
	gb(Declaration) >> Statement;

    gb(FunDecl) >> "fun" & Function | [&](auto& vals){ return vals[1]; };
	
    gb(VarDecl) >> "var" & Identifier & ";" | [&](auto& vals){ return std::make_shared<Prox::Stmt>(Prox::VariableStmt{vals[1].as<Prox::Ident>()}); };
	gb(VarDecl) >> "var" & Identifier & "=" & Expression & ";" | [&](auto& vals){ return std::make_shared<Prox::Stmt>(Prox::VariableStmt{vals[1].as<Prox::Ident>(), vals[3].as<Prox::ExprPtr>()}); };

	gb(Statement) >> ExprStmt;
	gb(Statement) >> IfStmt;
	gb(Statement) >> ReturnStmt;
	gb(Statement) >> WhileStmt;
	gb(Statement) >> Block | [&](auto& vals){ return std::make_shared<Prox::Stmt>(vals[0].as<Prox::BlockStmt>()); };
	
    gb(ExprStmt) >> Expression & ";" | [&](auto& vals){ return std::make_shared<Prox::Stmt>(Prox::ExprStmt{*vals[0].as<Prox::ExprPtr>()}); };

    gb(IfStmt) >> "if" & "(" & Expression & ")" & Statement | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::IfStmt{*vals[2].as<Prox::ExprPtr>(), vals[4].as<Prox::StmtPtr>(), Prox::StmtPtr{}});
    };
    gb(IfStmt) >> "if" & "(" & Expression & ")" & Statement & "else" & Statement | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::IfStmt{*vals[2].as<Prox::ExprPtr>(), vals[4].as<Prox::StmtPtr>(), vals[6].as<Prox::StmtPtr>()});
    };

    gb(ReturnStmt) >> "return" & Expression & ";" | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::ReturnStmt{*vals[1].as<Prox::ExprPtr>()});
    };

    gb(WhileStmt) >> "while" & "(" & Expression & ")" & Statement | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::WhileStmt{*vals[2].as<Prox::ExprPtr>(), vals[4].as<Prox::StmtPtr>()});
    };

    gb(Block) >> "{" & Declarations & "}" | [&](auto& vals){ return vals[1].as<Prox::BlockStmt>(); };

    gb(Expression) >> Assignment;

    gb(Assignment) >> Identifier & "=" & Assignment | [&](auto& vals)
    { 
        return std::make_shared<Prox::Expr>(Prox::AssignExpr{vals[0].as<Prox::Ident>(), vals[2].as<Prox::ExprPtr>()});
    };
    gb(Assignment) >> LogicOr;

    gb(LogicOr) >> LogicAnd;
    gb(LogicOr) >> LogicOr & "||" & LogicAnd | makeBinaryAction(Prox::BinaryExpr::Op::Or);

    gb(LogicAnd) >> Equality;
    gb(LogicAnd) >> LogicAnd & "&&" & Equality | makeBinaryAction(Prox::BinaryExpr::Op::And);

    gb(Equality) >> Comparison;
    gb(Equality) >> Equality & "!=" & Comparison | makeBinaryAction(Prox::BinaryExpr::Op::BangEqual);
    gb(Equality) >> Equality & "==" & Comparison | makeBinaryAction(Prox::BinaryExpr::Op::EqualEqual);

    gb(Comparison) >> Term;
    gb(Comparison) >> Comparison & ">" & Term | makeBinaryAction(Prox::BinaryExpr::Op::Greater);
    gb(Comparison) >> Comparison & ">=" & Term | makeBinaryAction(Prox::BinaryExpr::Op::GreaterEqual);
    gb(Comparison) >> Comparison & "<" & Term | makeBinaryAction(Prox::BinaryExpr::Op::Less);
    gb(Comparison) >> Comparison & "<=" & Term | makeBinaryAction(Prox::BinaryExpr::Op::LessEqual);

    gb(Term) >> Factor;
    gb(Term) >> Term & "-" & Factor | makeBinaryAction(Prox::BinaryExpr::Op::Sub);
    gb(Term) >> Term & "+" & Factor | makeBinaryAction(Prox::BinaryExpr::Op::Add);

    gb(Factor) >> Unary;
    gb(Factor) >> Factor & "/" & Unary | makeBinaryAction(Prox::BinaryExpr::Op::Div);
    gb(Factor) >> Factor & "*" & Unary | makeBinaryAction(Prox::BinaryExpr::Op::Mul);

    gb(Unary) >> Call;
    gb(Unary) >> "-" & Unary | [](auto& vals)
    {
        return std::make_shared<Prox::Expr>(Prox::UnaryExpr{Prox::UnaryExpr::Op::Minus, vals[1].as<Prox::ExprPtr>()});
    };
    gb(Unary) >> "!" & Unary | [](auto& vals)
    {
        return std::make_shared<Prox::Expr>(Prox::UnaryExpr{Prox::UnaryExpr::Op::Bang, vals[1].as<Prox::ExprPtr>()});
    };

    gb(Call) >> Primary;
    gb(Call) >> Identifier & "(" & ")" | [](auto& vals)
    {
        return std::make_shared<Prox::Expr>(Prox::CallExpr{vals[0].as<Prox::Ident>()});
    };
    gb(Call) >> Identifier & "(" & Arguments & ")" | [](auto& vals)
    {
        return std::make_shared<Prox::Expr>(Prox::CallExpr{vals[0].as<Prox::Ident>(), vals[2].as<Prox::Exprs>()});
    };

    gb(Primary) >> "null"       | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::LiteralExpr{Prox::Value{Prox::Null{}}}); };
    gb(Primary) >> "true"       | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::LiteralExpr{Prox::Value{1.f}}); };
    gb(Primary) >> "false"      | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::LiteralExpr{Prox::Value{0.f}}); };
    gb(Primary) >> Number       | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::LiteralExpr{vals[0].as<float>()}); };
    gb(Primary) >> String       | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::LiteralExpr{vals[0].as<std::string>()}); };
    gb(Primary) >> Identifier   | [&](auto& vals){ return std::make_shared<Prox::Expr>(Prox::VariableExpr{vals[0].as<Prox::Ident>()}); };
    gb(Primary) >> "(" & Expression & ")" | [&](auto& vals){ return vals[1]; };

    gb(Function) >> Identifier & "(" & ")" & Block | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::FuncStmt{vals[0].as<Prox::Ident>(), Prox::Parameters{}, vals[3].as<Prox::BlockStmt>()});
    };
    gb(Function) >> Identifier & "(" & Parameters & ")" & Block | [&](auto& vals)
    { 
        return std::make_shared<Prox::Stmt>(Prox::FuncStmt{vals[0].as<Prox::Ident>(), vals[2].as<Prox::Parameters>(), vals[4].as<Prox::BlockStmt>()}); 
    };

    gb(Parameters) >> Identifier                    | makeArrayStart.template operator()<Prox::Ident>();
    gb(Parameters) >> Parameters & "," & Identifier | makeArrayAppend.template operator()<Prox::Ident>();

    gb(Arguments) >> Expression                     | makeArrayStart.template operator()<Prox::ExprPtr>();
    gb(Arguments) >> Arguments & "," & Expression   | makeArrayAppend.template operator()<Prox::ExprPtr>();
    
	gb(Whitespace);
	gb(Whitespace) >> Regex{"\\s+"};

    gb(Number) >> Regex{"[0-9]+(\\.[0-9]+)?"} | [](auto& vals)
	{ 
		auto src = vals[0].src;
		float result;
		std::from_chars(src.data(), src.data() + src.size(), result);
		return result;
	};

    gb(String) >> Regex("\\\"[^\"]*\\\"") | [](auto& vals)
	{ 
        const auto& src = vals[0].src;
		return std::string{src.data() + 1, src.data() + src.size() - 1};
	};

    gb(Identifier) >> Regex{"[a-zA-Z_][a-zA-Z_0-9]*"} | [](auto& vals)
	{ 
		return Prox::Ident{vals[0].src};
	};
	
// clang-format on

	return gb.makeParser();
}

int main()
{
    auto parser = makeParser();

    std::string str =
 R"(
        fun fibonacci(num)
        {
          if (num <= 1) return 1;

          return fibonacci(num - 1) + fibonacci(num - 2);
        }

        fun main()
        {
            println("hello world" || print("notshown"));
            println(11 || 0);
            println(0 || 22);
            println(33 && 0);
            println(0 && 44);

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
)";

    if (auto result = parser.parse(str); result.has_value())
    {
        auto& v = result.as<std::vector<Prox::StmtPtr>>();
        Prox::Runner runner;
        runner.run(Prox::Stmt{v});
    }
    else
    {
        parser.printError();
    }
}