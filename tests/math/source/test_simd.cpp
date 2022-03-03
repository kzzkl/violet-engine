#include "test_common.hpp"

using namespace ash::math;

namespace ash::test
{
TEST_CASE("get mask", "[simd]")
{
    auto v1 = simd::get_mask<0x1101>();
    auto v2 = simd::set(0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF);

    float4 f1;
    simd::store(v1, f1);

    float4 f2;
    simd::store(v2, f2);
}
} // namespace ash::test