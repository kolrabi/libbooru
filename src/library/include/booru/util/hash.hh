#pragma once

#include <booru/common.hh>

// Hash functions for different types of hashes. These are meant for uniquely identifying
// files, not for cryptographically secure operations.
namespace Booru::Hash
{

// Digest a range of bytes into a digest hash value. TValue must have a static method `Digest` that
// takes a byte span and returns a digest hash value.
template <class THash>
static inline THash::Sum Digest( ByteSpan const& _Message )
{
    return THash::Digest( _Message );
}

// Helper function to digest a string into a digest hash value. TValue must have a static method
// `Digest` that takes a byte span and returns a digest hash value.
template <class THash>
static inline THash::Sum Digest( StringView const& _Message )
{
    ByteSpan msgSpan( (uint8_t const*)_Message.data(),
                      (uint8_t const*)_Message.data() + _Message.size() );
    return Digest<THash>( msgSpan );
}

// Class representing a round state for a hash function
template <class TValue, int N>
class RoundState
{
  public:
    // Default constructor
    constexpr RoundState() : state{} {}

    // Constructor with inital state
    constexpr RoundState( auto... args ) : state{ args... } {}

    // Element wise addition operator
    RoundState& operator+=( RoundState _Rhs )
    {
        for ( size_t i = 0; i < N; ++i )
        {
            state[i] += _Rhs.state[i];
        }
        return *this;
    }

    // Element accessor
    TValue& operator[]( int i ) { return state[i]; }

    // Round and digest specific non linear functions
    TValue BitSelBCD() const { return ( state[1] & state[2] ) | ( ~state[1] & state[3] ); }
    TValue BitSelDBC() const { return ( state[3] & state[1] ) | ( ~state[3] & state[2] ); }
    TValue BitXorBCD() const { return ( state[1] ^ state[2] ^ state[3] ); }
    TValue BitMajBCD() const
    {
        return ( state[1] & state[2] ) ^ ( state[1] & state[3] ) ^ ( state[2] & state[3] );
    }
    TValue BitIBCD() const { return state[2] ^ ( state[1] | ~state[3] ); }

  private:
    Array<TValue, N> state;
};

// Pad a message to a multiple of a block length, appends a 1 bit and a size field (64bit big endian
// of number of bits).
static inline ByteVector PadMessage( ByteSpan const& _Message, uint32_t _BlockLength )
{
    assert( _BlockLength > 0 );

    uint64_t numMsgBytes = _Message.size(); // number of bytes in the message
    uint32_t numMsgBlocks =
        ( ( numMsgBytes + 9 ) / _BlockLength ) +
        1; // number of blocks in the message plus 8 byte size, each block is 64 bytes
    uint32_t numTotalBytes =
        numMsgBlocks *
        _BlockLength; // number of total bytes in the message, including padding, size

    ByteVector paddedMessage;
    paddedMessage.resize( numTotalBytes, 0x00 );

    std::copy( std::begin( _Message ), std::end( _Message ), std::begin( paddedMessage ) );

    paddedMessage[numMsgBytes] = 0x80;

    uint64_t numMsgBits = numMsgBytes * 8;
    for ( size_t i = 0; i < 8; i++ )
    {
        paddedMessage.at( numTotalBytes - 8 + i ) = numMsgBits & 0xff;
        numMsgBits >>= 8;
    }

    assert( ( paddedMessage.size() % _BlockLength ) == 0 );
    return paddedMessage;
}

// Message Digest 5 hashing algorithm.
class MD5 final
{
  public:
    using Sum = MD5Sum;
    MD5()     = delete;

    static Sum Digest( ByteSpan const& _Message )
    {
        ByteVector paddedMessage    = PadMessage( _Message, BLOCK_LENGTH );
        uint32_t const numMsgBlocks = paddedMessage.size() / BLOCK_LENGTH;
        State state                 = INITIAL_STATE;

        uint32_t idxMsgByte = 0;
        for ( uint32_t idxBlock = 0; idxBlock < numMsgBlocks; idxBlock++ )
        {
            Array<uint32_t, 16> values;
            for ( uint32_t idxByte = 0; idxByte < BLOCK_LENGTH; idxMsgByte++, idxByte++ )
            {
                // update block word, shift in byte at msb
                uint32_t byte        = paddedMessage.at( idxMsgByte );
                values[idxByte >> 2] = ( byte << 24 ) | ( values[idxByte >> 2] >> 8 );
            }

            State oldState = state;

            // process block
            for ( uint32_t idxByte = 0; idxByte < BLOCK_LENGTH; idxByte++ )
            {
                uint32_t div16  = idxByte / 16;
                uint32_t f      = 0;
                uint32_t idxBuf = idxByte;
                switch ( div16 )
                {
                case 0:
                    f      = state.BitSelBCD();
                    idxBuf = idxByte;
                    break;
                case 1:
                    f      = state.BitSelDBC();
                    idxBuf = ( idxByte * 5 + 1 ) & 0x0f;
                    break;
                case 2:
                    f      = state.BitXorBCD();
                    idxBuf = ( idxByte * 3 + 5 ) & 0x0f;
                    break;
                case 3:
                    f      = state.BitIBCD();
                    idxBuf = ( idxByte * 7 + 0 ) & 0x0f;
                    break;
                }

                uint32_t t =
                    state[1] + std::rotl( state[0] + f + K[idxByte] + values[idxBuf], S[idxByte] );
                state[0] = state[3];
                state[3] = state[2];
                state[2] = state[1];
                state[1] = t;
            }

            state += oldState;
        }

        // put state into result
        Sum result;
        for ( int i = 0; i < 16; i++ )
        {
            result.at( i ) = ( state[i / 4] >> ( ( i % 4 ) * 8 ) ) & 0xff;
        }

        return result;
    }

  private:
    using State                        = RoundState<uint32_t, 4>;
    static uint32_t const BLOCK_LENGTH = 64;
    static State constexpr INITIAL_STATE{ 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u };

    static Array<uint32_t, 64> constexpr S{
        7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 7,  12, 17, 22, 5,  9,  14, 20, 5,  9,
        14, 20, 5,  9,  14, 20, 5,  9,  14, 20, 4,  11, 16, 23, 4,  11, 16, 23, 4,  11, 16, 23,
        4,  11, 16, 23, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21, 6,  10, 15, 21 };

    static Array<uint32_t, 64> constexpr K{
        0XD76AA478ul, 0XE8C7B756ul, 0X242070DBul, 0XC1BDCEEEul, 0XF57C0FAFul, 0X4787C62Aul,
        0XA8304613ul, 0XFD469501ul, 0X698098D8ul, 0X8B44F7AFul, 0XFFFF5BB1ul, 0X895CD7BEul,
        0X6B901122ul, 0XFD987193ul, 0XA679438Eul, 0X49B40821ul, 0XF61E2562ul, 0XC040B340ul,
        0X265E5A51ul, 0XE9B6C7AAul, 0XD62F105Dul, 0X02441453ul, 0XD8A1E681ul, 0XE7D3FBC8ul,
        0X21E1CDE6ul, 0XC33707D6ul, 0XF4D50D87ul, 0X455A14EDul, 0XA9E3E905ul, 0XFCEFA3F8ul,
        0X676F02D9ul, 0X8D2A4C8Aul, 0XFFFA3942ul, 0X8771F681ul, 0X6D9D6122ul, 0XFDE5380Cul,
        0XA4BEEA44ul, 0X4BDECFA9ul, 0XF6BB4B60ul, 0XBEBFBC70ul, 0X289B7EC6ul, 0XEAA127FAul,
        0XD4EF3085ul, 0X04881D05ul, 0XD9D4D039ul, 0XE6DB99E5ul, 0X1FA27CF8ul, 0XC4AC5665ul,
        0XF4292244ul, 0X432AFF97ul, 0XAB9423A7ul, 0XFC93A039ul, 0X655B59C3ul, 0X8F0CCC92ul,
        0XFFEFF47Dul, 0X85845DD1ul, 0X6FA87E4Ful, 0XFE2CE6E0ul, 0XA3014314ul, 0X4E0811A1ul,
        0XF7537E82ul, 0XBD3AF235ul, 0X2AD7D2BBul, 0XEB86D391ul };
};

// Secure hashing algorithm
class SHA1 final
{
  public:
    using Sum = SHA1Sum;
    SHA1() = delete;

    static Sum Digest( ByteSpan const& _Message )
    {
        Vector<uint8_t> paddedMessage = PadMessage( _Message, BLOCK_LENGTH );
        uint32_t const numMsgBlocks   = paddedMessage.size() / BLOCK_LENGTH;
        State state                   = INITIAL_STATE;

        uint32_t idxMsgByte = 0;
        for ( uint32_t idxBlock = 0; idxBlock < numMsgBlocks; idxBlock++ )
        {
            Array<uint32_t, 80> values{ 0 };

            for ( uint32_t idxByte = 0; idxByte < BLOCK_LENGTH; idxMsgByte++, idxByte++ )
            {
                uint32_t idxValue = idxByte / 4;
                values[idxValue] |= paddedMessage[idxMsgByte] << ( ( 3 - ( idxByte % 4 ) ) * 8 );
            }
            for ( uint32_t idxValue = 16; idxValue < 80; idxValue++ )
            {
                values[idxValue] = std::rotl( values[idxValue - 3] ^ values[idxValue - 8] ^
                                                  values[idxValue - 14] ^ values[idxValue - 16],
                                              1 );
            }

            State roundState = state;

            for ( uint32_t idxValue = 0; idxValue < 80; idxValue++ )
            {
                uint32_t n = idxValue / 20;
                uint32_t f = 0;
                switch ( n )
                {
                case 0:
                    f = roundState.BitSelBCD();
                    break;
                case 1:
                    f = roundState.BitXorBCD();
                    break;
                case 2:
                    f = roundState.BitMajBCD();
                    break;
                case 3:
                    f = roundState.BitXorBCD();
                    break;
                }

                uint32_t t =
                    ( std::rotl( roundState[0], 5 ) + f + roundState[4] + K[n] + values[idxValue] );
                roundState[4] = roundState[3];
                roundState[3] = roundState[2];
                roundState[2] = std::rotl( roundState[1], 30 );
                roundState[1] = roundState[0];
                roundState[0] = t;
            }

            state += roundState;
        }

        // put state into result
        Sum result;
        for ( int i = 0; i < 20; i++ )
        {
            result.at( i ) = ( state[i / 4] >> ( ( 3 - ( i % 4 ) ) * 8 ) ) & 0xff;
        }
        return result;
    }

  private:
    using State                        = RoundState<uint32_t, 5>;
    static uint32_t const BLOCK_LENGTH = 64;
    static State constexpr INITIAL_STATE{ 0x67452301u, 0xEFCDAB89u, 0x98BADCFEu, 0x10325476u,
                                          0xC3D2E1F0u };

    static Array<uint32_t, 4> constexpr K{ 0x5A827999u, 0x6ED9EBA1u, 0x8F1BBCDCu, 0xCA62C1D6u };
};

} // namespace Booru::Hash
