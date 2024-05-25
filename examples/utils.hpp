#pragma once

#include "magic_enum/magic_enum_all.hpp"

template<typename T>
std::string enumToString(T t)
{
    return std::string{magic_enum::enum_name(t)};
}