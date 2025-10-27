#pragma once

#include <booru/common.hh>

namespace Booru
{

int64_t SQLGetSchemaVersion();
StringView SQLGetBaseSchema();
StringView SQLGetUpdateSchema(int64_t _Version);

} // namespace Booru
