#pragma once

#ifdef _DEBUG
#    include <cassert>
#    define ASH_ASSERT(condition, ...) assert(condition)
#else
#    define ASH_ASSERT(condition, ...)
#endif