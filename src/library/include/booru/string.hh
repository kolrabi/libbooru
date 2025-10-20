#pragma once

#include <booru/common.hh>

#include <sstream>

// formatter for optional values
template<class TValue>
struct std::formatter<Booru::Optional<TValue>>
{
	template<typename ParseContext>
	constexpr auto parse(ParseContext& _Ctx)
	{
		return _Ctx.begin();
	}

	template<typename FormatContext>
	auto format(const Booru::Optional<TValue>& _Opt, FormatContext& _Ctx) const
	{
        if (_Opt.has_value())
    		return format_to(_Ctx.out(), "{}", _Opt.value());
		return format_to(_Ctx.out(), "<null>");
	}
};

template<class TValue>
std::ostream & operator<<(std::ostream & _Stream, Booru::Optional<TValue> _Value )
{
    return _Stream << std::format( "{}", _Value );
}

namespace Booru::Strings
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

/// @brief Build a hexadecimal string from a number of bytes.
/// @param _Data Data bytes to convert.
/// @return A hexadecimal representation of the given strings.
String From( ByteSpan const & _Data );

/// @brief Parse a hexadecimal string into a byte array.
/// @param _Hex Hexadecimal string to parse.
/// @return A vector containing the bytes from the string, or an error code.
Expected<ByteVector> ParseHex( StringView const & _Hex );

/// @brief Convert a byte array to a hexadecimal string.
/// Specialization for fixed arrays. Calls From( ByteSpan ).
/// @param _Data Data bytes to convert. 
/// @return A hexadecimal representation of the given data.
template <size_t N>
static inline String From( ByteArray<N> const & _Data )
{
    return From( ByteSpan(_Data) );
}

/// @brief Convert a byte array to a hexadecimal string.
/// Specialization for dynamic arrays. Calls From( ByteSpan ).
/// @param _Data Data bytes to convert. 
/// @return A hexadecimal representation of the given data.
static inline String From( ByteVector const & _Data )
{
    return From( ByteSpan(_Data) );
}



/// @brief General template for converting types to string. Uses operator<< of the type
/// @tparam TValue Type of value to convert.
/// @param _Item Value to convert.
/// @return A string representing the given value.
template <class TValue>
static inline String From( TValue const & _Item )
{
    std::stringstream ss;
    ss << _Item;
    return ss.str();
}

/// @brief Convert an optional value to a string.
/// @param _Item Optional value to convert. 
/// @return A string representation of the given optional value. 
/// If the optional value is unset, returns "<null>".
template <class TValue>
static inline String From( Optional<TValue> & _Item )
{
    if ( _Item.has_value() )
    {
        return From( _Item.value() );
    }
    return "<null>";
}

template <class TValue>
struct XFormFrom
{
    String operator() (TValue const & _In) const { return From(_In); }
};

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
    std::transform( std::begin( _Items ), std::end( _Items ), std::begin( strings ), _Op );
    return Join( strings, _Sep );
}

} // namespace Booru::Strings
