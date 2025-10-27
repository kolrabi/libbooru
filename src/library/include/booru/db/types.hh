#pragma once

#include <booru/common.hh>
#include <booru/string.hh>

namespace Booru::DB
{

// forward declarations

class IBackend;
using DBPtr      = Shared<IBackend>;
using ExpectedDB = Expected<DBPtr>;

class IStmt;
using StmtPtr                          = Shared<IStmt>;
using ExpectedStmt                     = Expected<StmtPtr>;

// type aliases

using INTEGER                          = int64_t;
using TEXT                             = String;
using FLOAT                            = double;
template <int N> using BLOB            = ByteArray<N>;
template <class TValue> using NULLABLE = Optional<TValue>;
using MD5BLOB                          = MD5Sum;

} // namespace Booru::DB
