#pragma once

#include "booru/common.hh"

static const int k_SQLSchemaVersion = 0;
extern char const* g_SQLSchema;

char const* SQLGetUpdateSchema( int64_t _Version );
