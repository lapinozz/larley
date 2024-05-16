#pragma once

namespace larley
{

template <typename ParserTypes>
struct Rule
{
    using Symbol = std::variant<typename ParserTypes::NonTerminal, typename ParserTypes::Terminal>;

    ParserTypes::NonTerminal product;
    std::vector<Symbol> symbols;

    std::size_t id;

    bool isEmpty() const
    {
        return symbols.size() == 0;
    }
};

template <typename ParserTypes>
struct Grammar
{
    using NT = ParserTypes::NonTerminal;
    using LT = ParserTypes::Terminal;
    using RuleT = Rule<ParserTypes>;

    Grammar(NT startSymbol, std::vector<RuleT> rules) : startSymbol{startSymbol}, rules{std::move(rules)}, nullables{makeNullableSet()}
    {
        check();
    }

    NT startSymbol;
    std::vector<RuleT> rules;
    std::unordered_set<NT> nullables;

    bool isNullable(const RuleT& rule) const
    {
        for (const auto& symbol : rule.symbols)
        {
            if (std::holds_alternative<NT>(symbol) && nullables.contains(std::get<NT>(symbol)))
            {
                continue;
            }

            return false;
        }

        return true;
    }

    // https://github.com/jeffreykegler/old_kollos/blob/master/notes/misc/loup2.md
    auto makeNullableSet() const
    {
        std::unordered_map<NT, std::unordered_set<const RuleT*>> lhsToRules;
        std::unordered_map<NT, std::unordered_set<const RuleT*>> rhsToRules;
        std::unordered_set<NT> nullables;

        for (const auto& rule : rules)
        {
            lhsToRules[rule.product].insert(&rule);

            for (const auto& symbol : rule.symbols)
            {
                if (symbol.index() == 0)
                {
                    rhsToRules[std::get<0>(symbol)].insert(&rule);
                }
            }

            if (rule.isEmpty())
            {
                nullables.insert(rule.product);
            }
        }

        std::vector<NT> workStack(nullables.begin(), nullables.end());
        while (workStack.size() > 0)
        {
            const auto workSymbol = workStack.back();
            workStack.pop_back();

            for (const auto& workRule : rhsToRules[workSymbol])
            {
                if (nullables.contains(workRule->product))
                {
                    continue;
                }

                const bool allNull = std::ranges::all_of(workRule->symbols, [&](const auto& symbol)
                                                         { return symbol.index() == 0 && nullables.contains(std::get<0>(symbol)); });

                if (!allNull)
                {
                    continue;
                }

                nullables.insert(workRule->product);
                workStack.push_back(workRule->product);
            }
        }

        return nullables;
    }

    bool check() const
    {
        const auto iter = [&](this auto const& iter, std::vector<NT>& path) -> void
        {
            const auto nt = path.back();

            for (const auto& rule : rules)
            {
                if (rule.product == nt && isNullable(rule))
                {
                    for (const auto& symbol : rule.symbols)
                    {
                        const auto n = std::get<NT>(symbol);

                        if (std::ranges::contains(path, n))
                        {
                            // std::throw_with_nested(std::logic_error("invalid grammar, recursive nullable"));
                            throw std::logic_error("invalid grammar, recursive nullable");
                            return;
                        }

                        path.push_back(n);
                        iter(path);
                        path.pop_back();
                    }
                }
            }
        };

        std::vector<NT> path;
        for (const auto nt : nullables)
        {
            path.clear();
            path.push_back(nt);
            iter(path);
        }

        return true;
    }
};

} // namespace larley