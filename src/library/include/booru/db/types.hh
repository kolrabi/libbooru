#pragma once

#include "booru/common.hh"
#include "booru/string.hh"

using INTEGER = int64_t;
using TEXT    = String;
using FLOAT   = double;
template <int N>
using BLOB = std::array<uint8_t, N>;
template <class TValue>
using NULLABLE = std::optional<TValue>;
using MD5BLOB  = MD5Sum;
