#pragma once

#ifndef NDEBUG
#    include <cassert>
#    define ASH_ASSERT(condition, ...) assert(condition)
#else
#    define ASH_ASSERT(condition, ...)
#endif