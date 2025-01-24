#pragma once

#include "booru/common.hh"

#include <sstream>
#include <string>
#include <vector>

using String     = std::string;
using StringList = std::vector<String>;

StringList SplitString( char const* _Str, char _Delim = ' ' );
String JoinString( StringList const& _Items, char const* _Sep = ", " );

String DataToHex( uint8_t const* _Data, size_t _Size );
ResultCode HexToData( char const* _Hex, std::vector<uint8_t>& _Data );

template <class TContainer>
String DataToHex( TContainer const& _Container )
{
    return DataToHex( _Container.data(), _Container.size() );
}

String MD5ToHex( MD5Sum const& _MD5 );
ResultCode HexToMD5( char const* _Hex, MD5Sum& _MD5 );

template <class TValue>
static inline String ToString( const TValue& _Item )
{
    std::stringstream ss;
    ss << _Item;
    return ss.str();
}

template <size_t N>
static inline String ToString( std::array<uint8_t, N> const& _Item )
{
    return DataToHex( _Item.data(), _Item.size() );
}

template <class TValue>
static inline String ToString( const std::optional<TValue>& _Item )
{
    if ( _Item.has_value() )
    {
        return ToString( _Item.value() );
    }
    return "null";
}

template <class TValue>
static inline String JoinString( std::vector<TValue> const& _Items, char const* _Sep = ", " )
{
    StringList Strings;
    for ( auto const& item : _Items )
    {
        Strings.push_back( ToString( item ) );
    }
    return JoinString( Strings, _Sep );
}
