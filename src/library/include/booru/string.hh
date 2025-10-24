#pragma once

#include <booru/common.hh>

#include <sstream>
namespace Booru
{

static inline uint8_t HexCharToInt( char _C )
{
    if ( _C >= '0' && _C <= '9' )
        return _C - '0';
    return 10 + ::toupper( _C ) - 'A';
}

static inline char IntToHexChar( uint8_t _I )
{
    static char const hex[] = "0123456789abcdef";
    return hex[_I % 16];
}

/// @brief Build a hexadecimal string from a number of bytes.
/// @param _Data Data bytes to convert.
/// @return A hexadecimal representation of the given strings.
template<class TValue>
String ToString( TValue const & _Value )
{
    std::stringstream ss;
    ss << _Value;
    return ss.str();
}

template<class TValue>
String ToString( Optional<TValue> const & _Value )
{
    return _Value.has_value() ? ToString(_Value.value()) : "<null>"s;
}

template<class TContainer> requires CByteContainer<TContainer>
static inline String ToString( TContainer const & _Data )
{
    // preallocate string, two digits per byte
    String str;
    str.reserve( _Data.size() * 2 );

    // convert each byte into digits and append
    for ( auto& ch : _Data )
    {
        str += IntToHexChar( ch >> 4 );
        str += IntToHexChar( ch & 0xf );
    }
    return str;
}

namespace Strings
{

/// @brief Splits a string based on a delimiter into a vector of strings.
/// @param _Str String to split.
/// @param _Delim Delimiter on which to split. Default is space.
/// @return A vector of strings. May contain empty strings in case of consecutive delimiters in _Str.
StringVector Split( StringView const & _Str, char _Delim = ' ' );

/// @brief Join a vector of strings into a single string with a separator. 
/// @param _Items Strings to concatenate.
/// @param _Sep Seperatator to use between the strings. Default is comma and space. 
/// @return A string containing all input strings separated by _Sep. If only one or zero input strings are given, no separator is used.
String Join( StringVector const& _Items, StringView const & _Sep = ", " );

/// @brief Parse a hexadecimal string into a byte array.
/// @param _Hex Hexadecimal string to parse.
/// @return A vector containing the bytes from the string, or an error code.
Expected<ByteVector> ParseHex( StringView const & _Hex );

/// @brief Operator for std::transform to create a String from a value.
template <class TValue>
struct XFormFrom
{
    String operator() (TValue const & _In) const { return ToString(_In); }
};

/// @brief Specialization of std::transform operator for things that already are Strings.
template<>
struct XFormFrom<String>
{
    String operator() (String const & _In) const { return _In; }
};


/// @brief Join a vector of value into a string. Converts each item into a string then
/// joins them together using the separator _Sep.
/// @param _Items Values to join together.
/// @param _Sep Separator between items.
/// @return A string containing all string representations of the given values separated by _Sep.
/// @see Join(StringVector)
template <class TContainer, typename FOp = XFormFrom<typename TContainer::value_type>>
static inline String JoinXForm( TContainer const& _Items, StringView const & _Sep = ", ",  FOp _Op = XFormFrom<typename TContainer::value_type>{} )
{
    StringVector strings;
    strings.resize( _Items.size() );
    std::ranges::transform( _Items, std::begin( strings ), _Op );
    return Join( strings, _Sep );
}

/// @brief Get a String that has all characters converted to lower case.
String ToLower( StringView const & _Str );

} // namespace Booru::Strings
}
