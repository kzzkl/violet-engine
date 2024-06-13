#include "components/mmd_morph.hpp"

namespace violet::sample
{
void mmd_morph::vertex_morph::evaluate(float weight, mmd_morph* morph)
{
    for (auto& [index, translate] : data)
    {
        float3& target = *(static_cast<float3*>(morph->vertex_morph_result->get_buffer()) + index);

        vector4 t1 = vector::load(translate);
        t1 = vector::mul(t1, weight);
        vector4 t2 = vector::load(target);
        t2 = vector::add(t1, t2);
        vector::store(t2, target);
    }
}

void mmd_morph::evaluate(std::size_t index, float weight)
{
    morphs[index]->evaluate(weight, this);
}
} // namespace violet::sample