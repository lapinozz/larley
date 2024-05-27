#pragma once

#include <iostream>
#include <string_view>

namespace larley
{

void printUnescaped(std::string_view string, std::ostream& os = std::cout)
{
    for (const char c : string)
    {
        switch (c)
        {
        case '\a':
            os << "\\a";
            break;
        case '\b':
            os << "\\b";
            break;
        case '\f':
            os << "\\f";
            break;
        case '\n':
            os << "\\n";
            break;
        case '\r':
            os << "\\r";
            break;
        case '\t':
            os << "\\t";
            break;
        case '\v':
            os << "\\v";
            break;
        case '\\':
            os << "\\\\";
            break;
        case '\'':
            os << "\\'";
            break;
        case '\"':
            os << "\\\"";
            break;
        case '\?':
            os << "\\\?";
            break;
        default:
            os << c;
        }
    }
}

template <class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

} // namespace larley