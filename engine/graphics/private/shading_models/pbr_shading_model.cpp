#include "graphics/shading_models/pbr_shading_model.hpp"
#include "graphics/render_device.hpp"
#include "graphics/resources/brdf_lut.hpp"

namespace violet
{
struct pbr_cs : public shading_model_cs
{
    static constexpr std::string_view path = "assets/shaders/shading/pbr.hlsl";

    struct constant_data
    {
        shading_model_cs::constant_data common;
        pbr_shading_model::constant_data extra;
    };
};

pbr_shading_model::pbr_shading_model()
    : shading_model(
          "PBR",
          {
              SHADING_GBUFFER_ALBEDO,
              SHADING_GBUFFER_MATERIAL,
              SHADING_GBUFFER_NORMAL,
              SHADING_GBUFFER_EMISSIVE,
          },
          {
              SHADING_AUXILIARY_BUFFER_DEPTH,
              SHADING_AUXILIARY_BUFFER_AO,
          })
{
}

const rdg_compute_pipeline& pbr_shading_model::get_pipeline()
{
    auto& device = render_device::instance();

    bool has_ao = has_auxiliary_buffer(SHADING_AUXILIARY_BUFFER_AO);

    if (has_ao && m_pipeline_without_ao.compute_shader == nullptr)
    {
        std::vector<std::wstring> defines = {L"-DUSE_AO_BUFFER"};
        m_pipeline_with_ao.compute_shader = device.get_shader<pbr_cs>(defines);
    }
    else if (!has_ao && m_pipeline_without_ao.compute_shader == nullptr)
    {
        m_pipeline_without_ao.compute_shader = device.get_shader<pbr_cs>();
    }

    return has_ao ? m_pipeline_with_ao : m_pipeline_without_ao;
}

pbr_shading_model::constant_data pbr_shading_model::get_constant() const
{
    return constant_data{
        .brdf_lut =
            render_device::instance().get_buildin_texture<brdf_lut>()->get_srv()->get_bindless(),
    };
}
} // namespace violet