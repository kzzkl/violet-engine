#pragma once

#ifndef NDEBUG
#    include <cassert>
#    define VIOLET_ASSERT(condition, ...) assert(condition)
#else
#    define VIOLET_ASSERT(condition, ...)
#endif