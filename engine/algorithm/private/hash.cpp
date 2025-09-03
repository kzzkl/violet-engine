#include "algorithm/hash.hpp"
#include <xxhash.h>

namespace violet
{
std::uint64_t hash::xx_hash(const void* data, std::size_t size)
{
    return XXH64(data, size, 0);
}
} // namespace violet