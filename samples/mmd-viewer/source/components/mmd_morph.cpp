#include "components/mmd_morph.hpp"

namespace violet::sample
{
void mmd_morph::vertex_morph::evaluate(float weight, mmd_morph* morph)
{
    for (auto& [index, translate] : data)
    {
        float3& target = *(static_cast<float3*>(morph->vertex_morph_result->get_buffer()) + index);

        float4_simd t1 = simd::load(translate);
        t1 = vector_simd::mul(t1, weight);
        float4_simd t2 = simd::load(target);
        t2 = vector_simd::add(t1, t2);
        simd::store(t2, target);
    }
}

void mmd_morph::evaluate(std::size_t index, float weight)
{
    morphs[index]->evaluate(weight, this);
}
} // namespace violet::sample