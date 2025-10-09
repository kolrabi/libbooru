#include <booru/string.hh>
#include <booru/result.hh>

namespace Booru::Strings
{

static constexpr String LOGGER = "booru.strings";

StringVector Split( StringView const & _Str, char _Delim )
{
    StringVector tokens;
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
    // preallocate string
    String::size_type reserveLength = _Sep.size() * _Items.size();
    for ( auto const& item : _Items )
    {
        reserveLength += item.size();
    }

    String str;
    str.reserve( reserveLength );

    // append items and separators
    bool first = true;
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

String From( ByteSpan const & _Data )
{
    // preallocate string, two digits per byte
    String str;
    str.reserve( _Data.size() * 2 );

    // convert each byte into digits and append
    for ( size_t i = 0; i < _Data.size(); i++ )
    {
        str += IntToHexChar( _Data[i] >> 4 );
        str += IntToHexChar( _Data[i] & 0xf );
    }
    return str;
}

Expected<ByteVector> ParseHex( StringView const & _Hex )
{
    ByteVector result;

    // Iterate over the string, each character represents a nybble.
    size_t strLength = _Hex.size();
    bool lowNybble = strLength % 2; // start on low nybble if string has odd number of digits
    
    uint8_t curByteValue = 0;
    for ( size_t i = 0; i < strLength; i++ )
    {
        // get next character, sanity check
        char ch = _Hex[i];
        if ( !std::isxdigit( ch ) )
            return ResultCode::InvalidArgument;

        // shift nybble value into the lower half of current value
        curByteValue = ( curByteValue << 4 ) | HexCharToInt( ch );
        if ( lowNybble )
        {
            // if we just shifted in the lower nybble, we're done with the byte.
            result.push_back( curByteValue );
            curByteValue = 0;
        }

        // next nybble
        lowNybble = !lowNybble;
    }

    if ( lowNybble )
    {
        // before the inversion of lowNybble we shifted in a high nybble, so we
        // need to finish the byte
        result.push_back( curByteValue << 4 );
    }
    return result;
}

} 
