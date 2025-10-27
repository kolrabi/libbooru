#include <booru/result.hh>
#include <booru/string.hh>

namespace Booru::Strings
{

StringVector Split(StringView const& _Str, char _Delim)
{
    StringVector tokens;
    std::stringstream ss(String{_Str});
    String token;
    while (std::getline(ss, token, _Delim))
    {
        tokens.push_back(token);
    }
    return tokens;
}

Expected<ByteVector> ParseHex(StringView const& _Hex)
{
    ByteVector result;

    // Iterate over the string, each character represents a nybble.
    size_t strLength = _Hex.size();
    bool lowNybble =
        strLength % 2; // start on low nybble if string has odd number of digits

    uint8_t curByteValue = 0;
    for (size_t i = 0; i < strLength; i++)
    {
        // get next character, sanity check
        char ch = _Hex[i];
        if (!std::isxdigit(ch)) return ResultCode::InvalidArgument;

        // shift nybble value into the lower half of current value
        curByteValue = (curByteValue << 4) | HexCharToInt(ch);
        if (lowNybble)
        {
            // if we just shifted in the lower nybble, we're done with the byte.
            result.push_back(curByteValue);
            curByteValue = 0;
        }

        // next nybble
        lowNybble = !lowNybble;
    }

    if (lowNybble)
    {
        // before the inversion of lowNybble we shifted in a high nybble, so we
        // need to finish the byte
        result.push_back(curByteValue << 4);
    }
    return result;
}

String ToLower(StringView const& _Str)
{
    String lower;
    lower.resize(_Str.size());
    std::ranges::transform(_Str, std::begin(lower),
                           [](char c) { return std::tolower(c); });
    return lower;
}

} // namespace Booru::Strings
