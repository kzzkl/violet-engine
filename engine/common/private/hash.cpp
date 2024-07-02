#include "common/hash.hpp"
#include "city_hash.hpp"

namespace violet
{
std::uint64_t hash::city_hash_64(const void* data, std::size_t size)
{
    return CityHash64(static_cast<const char*>(data), size);
}
} // namespace violet