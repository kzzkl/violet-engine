#include "ecs/storage.hpp"
#include "test_common.hpp"

using namespace violet::ecs;

namespace violet::test
{
TEST_CASE("chunk align", "[chunk]")
{
    auto c = std::make_unique<chunk>();

    uint64_t address = reinterpret_cast<uint64_t>(c.get());
    CHECK(address % 64 == 0);
}
} // namespace violet::test