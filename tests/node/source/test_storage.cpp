#include "test_common.hpp"

namespace violet::test
{
TEST_CASE("chunk align", "[chunk]")
{
    auto c = std::make_unique<core::archetype_storage::chunk>();

    uint64_t address = reinterpret_cast<uint64_t>(c.get());
    CHECK(address % 64 == 0);
}
} // namespace violet::test