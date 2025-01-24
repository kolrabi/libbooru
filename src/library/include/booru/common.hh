#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>
#include <array>

template <class TValue>
using Optional = std::optional<TValue>;

#include <memory>

template <class TValue>
using Owning = std::unique_ptr<TValue>;
template <class TValue>
using Shared = std::shared_ptr<TValue>;

enum class [[nodiscard]] ResultCode : int32_t;

using MD5Sum = std::array<uint8_t, 16>;
