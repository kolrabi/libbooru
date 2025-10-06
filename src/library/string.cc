#include <booru/string.hh>
#include <booru/result.hh>

namespace Booru::Strings
{

static constexpr String LOGGER = "booru.strings";

StringVector Split( StringView const & _Str, char _Delim )
{
    std::vector<String> tokens;
    std::stringstream ss( String{_Str} );
    String token;
    while ( std::getline( ss, token, _Delim ) )
    {
        tokens.push_back( token );
    }
    return tokens;
}

String Join( StringVector const& _Items, StringView const & _Sep )
{
    String::size_type reserveLength = _Sep.size() * _Items.size();
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
        str += From( item );
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

String BytesToHex( ByteSpan const & _Data )
{
    String str;
    str.reserve( _Data.size() * 2 );
    for ( size_t i = 0; i < _Data.size(); i++ )
    {
        str += IntToHexChar( _Data[i] >> 4 );
        str += IntToHexChar( _Data[i] & 0xf );
    }
    return str;
}

ResultCode HexToBytes( StringView const & _Hex, ByteVector & _Data )
{
    size_t l       = _Hex.size();
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

String From( MD5Sum const& _MD5 )
{ 
    return BytesToHex( ByteSpan( _MD5.data(), _MD5.size() ) );
}

ResultCode HexToMD5( StringView const & _Hex, MD5Sum& _MD5 )
{
    ByteVector data;
    CHECK_RETURN_RESULT_ON_ERROR( HexToBytes( _Hex, data ) );

    if ( data.size() < _MD5.size() )
        return ResultCode::ArgumentTooShort;
    if ( data.size() > _MD5.size() )
        return ResultCode::ArgumentTooLong;

    ::memcpy( _MD5.data(), data.data(), data.size() * sizeof( data[0] ) );
    return ResultCode::OK;
}

}
