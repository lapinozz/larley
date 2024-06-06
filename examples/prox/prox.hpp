#pragma once

#include <variant>
#include <string>
#include <memory>
#include <ranges>
#include <functional>

namespace Prox
{
    struct Null
    {
    };

	using Value = std::variant<Null, float, std::string>;

	using Ident = std::string;

	struct Expr;

	using ExprPtr = std::shared_ptr<Expr>;

    using Exprs = std::vector<ExprPtr>;

	struct BinaryExpr
	{
		enum class Op
		{
			Add,
			Sub,
			Mul,
			Div,
			Greater,
			GreaterEqual,
			Less,
			LessEqual,
			BangEqual,
			EqualEqual,
            Or,
            And
		};

		ExprPtr left;
        Op op;
        ExprPtr right;
    };

    struct UnaryExpr
    {
        enum class Op
        {
            Minus,
			Bang
        };

        Op op;
        ExprPtr value;
    };

    struct AssignExpr
    {
        Ident ident;
        ExprPtr value;
    };

    struct LiteralExpr
    {
        Value value;
    };

    struct VariableExpr
    {
        Ident ident;
    };

    struct CallExpr
    {
        Ident ident;
        Exprs arguments;
    };

    struct Expr : std::variant<BinaryExpr, UnaryExpr, AssignExpr, LiteralExpr, VariableExpr, CallExpr>
    {

    };

    struct Stmt;

    using StmtPtr = std::shared_ptr<Stmt>;

    using BlockStmt = std::vector<StmtPtr>;

    struct ExprStmt
    {
        Expr expr;
    };

    using Parameters = std::vector<Ident>;

    struct FuncStmt
    {
        Ident ident;
        std::vector<Ident> argument;
        BlockStmt body;
    };

    struct IfStmt
    {
        Expr condition;
        StmtPtr thenStmt;
        StmtPtr elseStmt;
    };

    struct WhileStmt
    {
        Expr condition;
        StmtPtr body;
    };

    struct ReturnStmt
    {
        Expr value;
    };

    struct VariableStmt
    {
        Ident ident;
        ExprPtr value;
    };

    struct Stmt : std::variant<BlockStmt, ExprStmt, FuncStmt, IfStmt, WhileStmt, ReturnStmt, VariableStmt>
    {
    };

    void printValue(const Value& value)
    {
        if (const auto* v = std::get_if<float>(&value))
        {
            std::cout << *v;
        }
        else if (const auto* v = std::get_if<std::string>(&value))
        {
            std::cout << *v;
        }
        else if (const auto* v = std::get_if<Null>(&value))
        {
            std::cout << "NULL";
        }
        else
        {
            assert(false);
        }
    }

    struct Runner
    {
        struct Scope
        {
            bool freshScope;

            std::unordered_map<Ident, Value> variables;
            std::unordered_map<Ident, FuncStmt> functions;
        };

        using BuiltInFunc = std::function<Value(const std::vector<Value>&)>;
        std::unordered_map<Ident, BuiltInFunc> builtInfunctions;

        Runner()
        {
            builtInfunctions["print"] = [](const std::vector<Value>& values)
            {
                for (const auto& value : values)
                {
                    printValue(value);
                    std::cout << " ";
                }

                return Null{};
            };

            builtInfunctions["println"] = [](const std::vector<Value>& values)
            {
                for (const auto& value : values)
                {
                    printValue(value);
                    std::cout << " ";
                }

                std::cout << "\n";

                return Null{};
            };
        }

        struct ScopeGuard
        {
            Runner& runner;

            ScopeGuard(Runner& runner, bool isFunc = false) : runner{runner}
            {
                runner.scopes.push_back({isFunc});
            }

            ~ScopeGuard()
            {
                runner.scopes.pop_back();
            }
        };

        std::vector<Scope> scopes;

        bool isTruthy(const Value& value)
        {
            if (const auto* v = std::get_if<float>(&value))
            {
                return *v != 0;
            }
            else if (const auto* v = std::get_if<std::string>(&value))
            {
                return !v->empty();
            }

            return false;
        }

        Value& getVar(const Ident& ident)
        {
            for (auto& scope : std::ranges::reverse_view(scopes))
            {
                auto it = scope.variables.find(ident);
                if (it != scope.variables.end())
                {
                    return it->second;
                }

                if (scope.freshScope)
                {
                    break;
                }
            }

            auto it = scopes.front().variables.find(ident);
            if (it != scopes.front().variables.end())
            {
                return it->second;
            }

            assert(false && "Can't find variable");
        }

        const FuncStmt& getFunc(const Ident& ident)
        {
            for (auto& scope : std::ranges::reverse_view(scopes))
            {
                auto it = scope.functions.find(ident);
                if (it != scope.functions.end())
                {
                    return it->second;
                }

                if (scope.freshScope)
                {
                    break;
                }
            }

            auto it = scopes.front().functions.find(ident);
            if (it != scopes.front().functions.end())
            {
                return it->second;
            }

            assert(false && "Can't find function");
        }

        std::optional<Value> run(const Stmt& stmt)
        {
            if (auto* s = std::get_if<BlockStmt>(&stmt))
            {
                ScopeGuard guard{*this};
                for (const auto& subStmt : *s)
                {
                    if (subStmt)
                    {
                        auto val = run(*subStmt);
                        if (val)
                        {
                            return val;
                        }
                    }
                }
            }
            else if (auto* s = std::get_if<ExprStmt>(&stmt))
            {
                evaluate(s->expr);
            }
            else if (auto* s = std::get_if<FuncStmt>(&stmt))
            {
                scopes.back().functions[s->ident] = *s;
            }
            else if (auto* s = std::get_if<IfStmt>(&stmt))
            {
                if (isTruthy(evaluate(s->condition)))
                {
                    ScopeGuard guard{*this};
                    return run(*s->thenStmt);
                }
                else if (s->elseStmt)
                {
                    ScopeGuard guard{*this};
                    return run(*s->elseStmt);
                }
            }
            else if (auto* s = std::get_if<WhileStmt>(&stmt))
            {
                while (isTruthy(evaluate(s->condition)))
                {
                    ScopeGuard guard{*this};
                    auto val = run(*s->body);
                    if (val)
                    {
                        return val;
                    }
                }
            }
            else if (auto* s = std::get_if<ReturnStmt>(&stmt))
            {
                return evaluate(s->value);
            }
            else if (auto* s = std::get_if<VariableStmt>(&stmt))
            {
                if (s->value)
                {
                    scopes.back().variables[s->ident] = evaluate(*s->value);
                }
                else
                {
                    scopes.back().variables[s->ident] = Value{Null{}};
                }
            }

            return std::nullopt;
        }

        Value evaluate(const Expr& expr)
        {
            if (auto* e = std::get_if<BinaryExpr>(&expr))
            {
                auto v1 = evaluate(*e->left);

                const auto arithmeticImpl = [&](const auto& op) -> Value
                {
                    auto v2 = evaluate(*e->right);
                    if (std::holds_alternative<float>(v1) && std::holds_alternative<float>(v2))
                    {
                        return static_cast<float>(op(std::get<float>(v1), std::get<float>(v2)));
                    }

                    assert(false && "invalid argument");
                };

                const auto compImpl = [&](const auto& op) -> Value
                {
                    auto v2 = evaluate(*e->right);
                    if (std::holds_alternative<float>(v1) && std::holds_alternative<float>(v2))
                    {
                        return op(std::get<float>(v1), std::get<float>(v2));
                    }

                    assert(false && "invalid argument");
                };

                if (e->op == BinaryExpr::Op::Add)
                {
                    auto v2 = evaluate(*e->right);
                    if (std::holds_alternative<float>(v1) && std::holds_alternative<float>(v2))
                    {
                        return std::get<float>(v1) + std::get<float>(v2);
                    }
                    else if (std::holds_alternative<std::string>(v1) && std::holds_alternative<std::string>(v2))
                    {
                        return std::get<std::string>(v1) + std::get<std::string>(v2);
                    }

                    assert(false && "invalid argument");
                }
                else if (e->op == BinaryExpr::Op::Sub)
                {
                    return arithmeticImpl(std::minus{});
                }
                else if (e->op == BinaryExpr::Op::Mul)
                {
                    return arithmeticImpl(std::multiplies{});
                }
                else if (e->op == BinaryExpr::Op::Div)
                {
                    return arithmeticImpl(std::divides{});
                }
                else if (e->op == BinaryExpr::Op::Greater)
                {
                    return arithmeticImpl(std::greater{});
                }
                else if (e->op == BinaryExpr::Op::GreaterEqual)
                {
                    return arithmeticImpl(std::greater_equal{});
                }
                else if (e->op == BinaryExpr::Op::Less)
                {
                    return arithmeticImpl(std::less{});
                }
                else if (e->op == BinaryExpr::Op::LessEqual)
                {
                    return arithmeticImpl(std::less_equal{});
                }
                else if (e->op == BinaryExpr::Op::EqualEqual)
                {
                    return arithmeticImpl(std::equal_to{});
                }
                else if (e->op == BinaryExpr::Op::BangEqual)
                {
                    return arithmeticImpl(std::not_equal_to{});
                }
                else if (e->op == BinaryExpr::Op::Or)
                {
                    bool b1 = isTruthy(v1);
                    if (b1)
                    {
                        return v1;
                    }

                    return evaluate(*e->right);

                    assert(false && "invalid argument");
                }
                else if (e->op == BinaryExpr::Op::And)
                {
                    bool b1 = isTruthy(v1);
                    if (!b1)
                    {
                        return v1;
                    }

                    return evaluate(*e->right);

                    assert(false && "invalid argument");
                }
            }
            else if (auto* e = std::get_if<UnaryExpr>(&expr))
            {
                auto value = evaluate(*e->value);

                if (e->op == UnaryExpr::Op::Bang)
                {
                    return static_cast<float>(!isTruthy(value));
                }
                else if (e->op == UnaryExpr::Op::Minus)
                {
                    if (auto* v = std::get_if<float>(&value))
                    {
                        return -*v;
                    }

                    assert(false && "invalid argument");
                }
            }
            else if (auto* e = std::get_if<AssignExpr>(&expr))
            {
                auto& var = getVar(e->ident);
                var = evaluate(*e->value);
                return var;
            }
            else if (auto* e = std::get_if<LiteralExpr>(&expr))
            {
                return e->value;
            }
            else if (auto* e = std::get_if<VariableExpr>(&expr))
            {
                return getVar(e->ident);
            }
            else if (auto* e = std::get_if<CallExpr>(&expr))
            {
                std::vector<Value> values;
                for (auto& expr : e->arguments)
                {
                    values.push_back(evaluate(*expr));
                }

                auto it = builtInfunctions.find(e->ident);
                if (it != builtInfunctions.end())
                {
                    return it->second(values);
                }

                auto func = getFunc(e->ident);
                if (func.argument.size() != e->arguments.size())
                {
                    assert(false && "argument missmatch");
                }

                ScopeGuard guard1{*this, true};

                for (std::size_t x = 0; x < e->arguments.size(); x++)
                {
                    scopes.back().variables[func.argument[x]] = std::move(values[x]);
                }

                return run({func.body}).value_or(Value{Null{}});
            }

            return {Null{}};
        }
    };
}