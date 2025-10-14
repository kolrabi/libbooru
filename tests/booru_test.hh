#pragma once

#include <cstdlib>

static inline void PASS()
{
    exit(EXIT_SUCCESS);
}

static inline void FAIL()
{
    exit(EXIT_FAILURE);
}
