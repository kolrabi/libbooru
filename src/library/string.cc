#include "booru/string.hh"

#include "booru/result.hh"

#include <cstring>
#include <array>

std::vector<String> SplitString( char const* _Str, char _Delim )
{
    std::vector<String> tokens;
    std::stringstream ss( _Str );
    String token;
    while ( std::getline( ss, token, _Delim ) )
    {
        tokens.push_back( token );
    }
    return tokens;
}

String JoinString( StringList const& _Items, char const* _Sep )
{
    String::size_type reserveLength = ::strlen( _Sep ) * _Items.size();
    for ( auto const& item : _Items )
    {
        reserveLength += item.size();
    }

    bool first = true;
    String str;
    str.reserve( reserveLength );
    for ( auto const& item : _Items )
    {
        if ( !first )
        {
            str += _Sep;
        }
        first = false;
        str += ToString( item );
    }
    return str;
}

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

String DataToHex( uint8_t const* _Data, size_t _Size )
{
    String str;
    str.reserve( _Size * 2 );
    for ( size_t i = 0; i < _Size; i++ )
    {
        str += IntToHexChar( _Data[i] >> 4 );
        str += IntToHexChar( _Data[i] & 0xf );
    }
    return str;
}

ResultCode HexToData( char const* _Hex, std::vector<uint8_t>& _Data )
{
    size_t l       = strlen( _Hex );
    bool lowNibble = l % 2;

    _Data.clear();

    uint8_t value = 0;
    for ( size_t i = 0; i < l; i++ )
    {
        char ch = _Hex[i];
        if ( !::isxdigit( ch ) )
            return ResultCode::InvalidArgument;

        value = ( value << 4 ) | HexCharToInt( ch );
        if ( lowNibble )
        {
            _Data.push_back( value );
            value = 0;
        }

        lowNibble = !lowNibble;
    }
    if ( lowNibble )
    {
        _Data.push_back( value );
    }
    return ResultCode::OK;
}

String MD5ToHex( MD5Sum const& _MD5 ) { return DataToHex( _MD5.data(), _MD5.size() ); }

ResultCode HexToMD5( char const* _Hex, MD5Sum& _MD5 )
{
    std::vector<uint8_t> data;
    CHECK_RESULT_RETURN_ERROR( HexToData( _Hex, data ) );

    if ( data.size() < _MD5.size() )
        return ResultCode::ArgumentTooShort;
    if ( data.size() > _MD5.size() )
        return ResultCode::ArgumentTooLong;

    ::memcpy( _MD5.data(), data.data(), data.size() * sizeof( data[0] ) );
    return ResultCode::OK;
}
