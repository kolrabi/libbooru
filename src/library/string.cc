#include <booru/string.hh>
#include <booru/result.hh>

namespace Booru::Strings
{

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
    // todo: ranges skip 
    for ( auto const& item : _Items )
    {
        if ( !first )
        {
            str += _Sep;
        }
        first = false;
        str += item;
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

String ToLower( StringView const & _Str )
{
    String lower;
    lower.resize(_Str.size());
    std::ranges::transform( _Str, std::begin(lower), []( char c ) { return std::tolower( c ); } );
    return lower;
}

} 
