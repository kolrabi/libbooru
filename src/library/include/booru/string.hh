#pragma once

#include <booru/common.hh>

#include <sstream>
namespace Booru
{

static inline uint8_t HexCharToInt(char _C)
{
    if (_C >= '0' && _C <= '9') return _C - '0';
    return 10 + ::toupper(_C) - 'A';
}

static inline char IntToHexChar(uint8_t _I)
{
    static char const hex[] = "0123456789abcdef";
    return hex[_I % 16];
}

/// @brief Build a hexadecimal string from a number of bytes.
/// @param _Data Data bytes to convert.
/// @return A hexadecimal representation of the given strings.
String ToString(CStreamPrintable auto const& _Value)
{
    std::stringstream ss;
    ss << _Value;
    return ss.str();
}

template <class TValue> String ToString(Optional<TValue> const& _Value)
{
    return _Value.has_value() ? ToString(_Value.value()) : "<null>"s;
}

static inline String ToString(CTypedConstIterable<Byte> auto const& _Data)
{
    // preallocate string, two digits per byte
    String str;
    str.reserve(_Data.size() * 2);

    // convert each byte into digits and append
    for (auto& ch : _Data)
    {
        str += IntToHexChar(ch >> 4);
        str += IntToHexChar(ch & 0xf);
    }
    return str;
}

namespace Strings
{

/// @brief Splits a string based on a delimiter into a vector of strings.
/// @param _Str String to split.
/// @param _Delim Delimiter on which to split. Default is space.
/// @return A vector of strings. May contain empty strings in case of
/// consecutive delimiters in _Str.
StringVector Split(StringView const& _Str, char _Delim = ' ');

/// @brief Join a vector of strings into a single string with a separator.
/// @param _Items Strings to concatenate.
/// @param _Sep Seperatator to use between the strings. Default is comma and
/// space.
/// @return A string containing all input strings separated by _Sep. If only one
/// or zero input strings are given, no separator is used.
String Join(CTypedConstIterable<String> auto const& _Items,
            StringView const& _Sep)
{
    // preallocate string
    String::size_type reserveLength = _Sep.size() * _Items.size();
    for (auto const& item : _Items)
    {
        reserveLength += item.size();
    }

    String str;
    str.reserve(reserveLength);

    // append items and separators
    bool first = true;
    // todo: ranges skip
    for (auto const& item : _Items)
    {
        if (!first) { str += _Sep; }
        first  = false;
        str   += item;
    }
    return str;
}

/// @brief Parse a hexadecimal string into a byte array.
/// @param _Hex Hexadecimal string to parse.
/// @return A vector containing the bytes from the string, or an error code.
Expected<ByteVector> ParseHex(StringView const& _Hex);

/// @brief Operator for std::transform to create a String from a value.
template <class TValue> struct XFormFrom
{
    String operator()(TValue const& _In) const { return ToString(_In); }
};

/// @brief Specialization of std::transform operator for things that already are
/// Strings.
template <> struct XFormFrom<String>
{
    String operator()(String const& _In) const { return _In; }
};

/// @brief Join a vector of value into a string. Converts each item into a
/// string then joins them together using the separator _Sep.
/// @param _Items Values to join together.
/// @param _Sep Separator between items.
/// @return A string containing all string representations of the given values
/// separated by _Sep.
/// @see Join(StringVector)
template <class TIterable,
          typename FOp = XFormFrom<typename TIterable::value_type>>
static inline String
JoinXForm(TIterable const& _Items, StringView const& _Sep = ", ",
          FOp _Op = XFormFrom<typename TIterable::value_type>{})
{
    StringVector strings;
    strings.resize(_Items.size());
    std::ranges::transform(_Items, std::begin(strings), _Op);
    return Join(strings, _Sep);
}

String Join(CStringConvertibleConstIterable auto const& _Items,
            StringView const& _Sep = ", ")
{
    return JoinXForm(_Items, _Sep, &ToString<decltype(_Items)::value_type>);
}

/// @brief Get a String that has all characters converted to lower case.
String ToLower(StringView const& _Str);

} // namespace Strings
} // namespace Booru
