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

String ConvertC32ToUTF8(char32_t _Ch32)
{
    String str;
    uint32_t c = static_cast<uint32_t>(_Ch32);

    if (c < 0x80) { str.push_back((char)c); }
    else if (c < 0x800)
    {
        str.push_back((char)(0b11000000 | ((c >> 6) & 0b00011111)));
        str.push_back((char)(0b10000000 | ((c >> 0) & 0b00111111)));
    }
    else if (c < 0x10000)
    {
        str.push_back((char)(0b11100000 | ((c >> 12) & 0b00001111)));
        str.push_back((char)(0b10000000 | ((c >> 6) & 0b00111111)));
        str.push_back((char)(0b10000000 | ((c >> 0) & 0b00111111)));
    }
    else if (c < 0x110000)
    {
        str.push_back((char)(0b11110000 | ((c >> 17) & 0b00000111)));
        str.push_back((char)(0b10000000 | ((c >> 12) & 0b00111111)));
        str.push_back((char)(0b10000000 | ((c >> 6) & 0b00111111)));
        str.push_back((char)(0b10000000 | ((c >> 0) & 0b00111111)));
    }
    return str;
}

String ConvertC32ToUTF8(std::u32string_view const& _Str32)
{
    String str;
    for (auto& Ch32 : _Str32)
        str += ConvertC32ToUTF8(Ch32);
    return str;
}

String ParseHTMLEntity(StringView const& _entity)
{
    if (_entity.starts_with("#x") || _entity.starts_with("#X"))
    {
        auto substr = _entity.substr(2);
        size_t ch32 = 0;
        auto [ptr, error] =
            std::from_chars(substr.begin(), substr.end(), ch32, 16);
        if (error != std::errc())
        {
            LOG_WARNING("Could not parse entity '{}'", _entity);
        }
        return ConvertC32ToUTF8(ch32);
    }
    else if (_entity[0] == '#')
    {
        auto substr = _entity.substr(1);
        size_t ch32 = 0;
        auto [ptr, error] =
            std::from_chars(substr.begin(), substr.end(), ch32, 10);
        if (error != std::errc())
        {
            LOG_WARNING("Could not parse entity '{}'", _entity);
        }
        return ConvertC32ToUTF8(ch32);
    }

    // TODO: this is incomplete
    static std::map<char const*, String> entities = {
        {"nbsp", ConvertC32ToUTF8(0x00A0)},   // non breaking space
        {"shy", ConvertC32ToUTF8(0x00AD)},    // soft hyphen
        {"deg", ConvertC32ToUTF8(0x00B0)},    // °
        {"sup1", ConvertC32ToUTF8(0x00B9)},   // ¹
        {"Atilde", ConvertC32ToUTF8(0x00C3)}, // Ã
        {"atilde", ConvertC32ToUTF8(0x00E3)}, // ã
        {"dagger", ConvertC32ToUTF8(0x2020)}, // †
        {"rsquo", ConvertC32ToUTF8(0x2019)},  // ’
        {"gt", ">"},
        {"lt", "<"},
        {"amp", "&"},
        {"quot", "\""},
        {"apos", "'"},
    };

    for (auto& [key, value] : entities)
    {
        if (_entity == key) { return value; }
    }

    LOG_ERROR("Unknown entity '&{};'", _entity);
    return String(_entity);
}

String Replace(StringView const& _Str, StringView const& _Pattern,
               StringView const& _Replace)
{
    String newString{_Str};
    size_t index = newString.find(_Pattern);
    if (index != String::npos)
    {
        newString.replace(index, _Pattern.length(), _Replace);
    }
    return newString;
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

Expected<ByteVector> ParseBase64(StringView const& _Base64)
{
    ByteVector result;
    result.reserve(_Base64.size() * 3 / 4);

    static StringView constexpr base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    static auto convert = [](ByteArray<4> const& _In, ByteVector& _Out)
    {
        ByteArray<4> bytes = _In;
        for (auto& byte : bytes)
            byte = base64Chars.find(byte);

        _Out.push_back(((bytes[0] & 0xff) << 2) | ((bytes[1] & 0x30) >> 4));
        _Out.push_back(((bytes[1] & 0x0f) << 4) | ((bytes[2] & 0x3c) >> 2));
        _Out.push_back(((bytes[2] & 0x03) << 6) | ((bytes[3] & 0xff) << 0));
    };

    size_t counter = 0;
    ByteArray<4> scratchPad;
    for (auto& ch : _Base64)
    {
        static auto isBase64 = [](Byte _C)
        { return (std::isalnum(_C) || (_C == '+') || (_C == '/')); };

        if (ch == '=') break;
        if (!isBase64(ch)) return ResultCode::InvalidArgument;

        scratchPad[counter] = ch;
        counter             = (counter + 1) % 4;
        if (counter == 0)
        {
            convert(scratchPad, result);
            scratchPad.fill(0);
        }
    }

    if (counter != 0)
    {
        convert(scratchPad, result);
        result.resize(result.size() - 4 + counter);
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
