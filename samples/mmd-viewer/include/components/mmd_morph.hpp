#pragma once

#include "graphics/render_interface.hpp"
#include "math/math.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet::sample
{
class mmd_morph
{
public:
    class morph
    {
    public:
        virtual ~morph() = default;

        virtual void evaluate(float weight, mmd_morph* morph) {}

        std::string name;
    };

    class vertex_morph : public morph
    {
    public:
        struct vertex_morph_data
        {
            std::int32_t index;
            float3 translate;
        };

        virtual void evaluate(float weight, mmd_morph* morph) override;

        std::vector<vertex_morph_data> data;
    };

    void evaluate(std::size_t index, float weight);

    std::vector<std::unique_ptr<morph>> morphs;

    rhi_buffer* vertex_morph_result;
};
} // namespace violet::sample