#pragma once

#include <booru/common.hh>

#include <sstream>

namespace Booru::Strings
{

StringVector Split( StringView const & _Str, char _Delim = ' ' );
String Join( StringVector const& _Items, StringView const & _Sep = ", " );

String BytesToHex( ByteSpan const & _Data );
ResultCode HexToBytes( StringView const & _Hex, ByteVector & _Data );

String From( MD5Sum const& _MD5 );
ResultCode HexToMD5( StringView const & _Hex, MD5Sum& _MD5 );

template <class TValue>
static inline String From( const TValue& _Item )
{
    std::stringstream ss;
    ss << _Item;
    return ss.str();
}

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
static inline String Join( Vector<TValue> const& _Items, StringView const & _Sep = ", " )
{
    StringVector strings;
    for ( auto const& item : _Items )
    {
        strings.push_back( From( item ) );
    }
    return Join( strings, _Sep );
}
} // namespace Booru
