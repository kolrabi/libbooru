#pragma once

#include <cstdint>

#include <optional>
#include <array>
#include <span>
#include <vector>
#include <memory>
#include <string>
#include <string_view>

namespace Booru
{

using namespace std::string_literals;
using namespace std::string_view_literals;

// forwad declarations 
class Booru;
enum class [[nodiscard]] ResultCode : int32_t;

// type aliases
using Byte = uint8_t;

template <class TValue>
using Optional = std::optional<TValue>;
template <class TValue, int N>
using Array = std::array<TValue, N>;
template <class TValue>
using Span = std::span<TValue>;
template <class TValue>
using Vector = std::vector<TValue>;
template <class TValue>
using Owning = std::unique_ptr<TValue>;
template <class TValue>
using Shared = std::shared_ptr<TValue>;

using String     = std::string;
using StringView = std::string_view;
using StringVector = std::vector<String>;

using ByteSpan   = Span<Byte const>;
using ByteVector = Vector<Byte>;

template<int N>
using ByteArray = Array<Byte, N>;

using MD5Sum  = ByteArray<16>;
using SHA1Sum = ByteArray<20>;

template <class TValue, class... Args>
static inline Owning<TValue> MakeOwning( Args... args )
{
    return std::make_unique<TValue>( args... );
}

} // namespace Booru
