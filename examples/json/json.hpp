#pragma once

#include <variant>

namespace Json
{
    class Data;

    using Value = std::shared_ptr<Data>;

	using Object = std::unordered_map<std::string, Value>;
    using Array = std::vector<Value>;
    using String = std::string;
    using Number = float;
    using Boolean = bool;

    struct Data : std::variant<Object, Array, String, Number, Boolean>
    {
        using variant::variant;
    };

    void printIndentation(std::ostream& stream, std::size_t indentation)
    {
        for (std::size_t x = 0; x < indentation; x++)
        {
            stream << "  ";
        }
    }

    void print(std::ostream& stream, const Value& value, std::size_t indentation)
    {
        if (!value)
        {
            stream << "null";
            return;
        }

        if (auto* obj = std::get_if<Object>(&*value))
        {
            stream << "{\n";

            std::size_t x = 0;
            for (const auto& member : *obj)
            {
                printIndentation(stream, indentation + 1);

                stream << '"' << member.first << "\": ";
                print(stream, member.second, indentation + 1);
                if (++x < obj->size())
                {
                    stream << ',';
                }

                stream << '\n';
            }

            printIndentation(stream, indentation);

            stream << "}";
        }
        else if (auto* array = std::get_if<Array>(&*value))
        {
            stream << "[\n";

            std::size_t x = 0;
            for (const auto& member : *array)
            {
                printIndentation(stream, indentation + 1);

                print(stream, member, indentation + 1);
                if (++x < array->size())
                {
                    stream << ',';
                }

                stream << '\n';
            }

            printIndentation(stream, indentation);

            stream << "]";
        }
        else if (auto* str = std::get_if<String>(&*value))
        {
            stream << '"' << *str << '"';
        }
        else if (auto* num = std::get_if<Number>(&*value))
        {
            stream << *num;
        }
        else if (auto* boolean = std::get_if<Boolean>(&*value))
        {
            stream << std::boolalpha << *boolean;
        }
    }

    std::ostream& operator<<(std::ostream& stream, const Value& value)
    {
        print(stream, value, 0);
        stream << "\n";
        return stream;
    }
 };