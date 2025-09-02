#include "graphics/shading_models/unlit_shading_model.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
struct unlit_cs : public shading_model_cs
{
    static constexpr std::string_view path = "assets/shaders/shading/unlit.hlsl";
};

unlit_shading_model::unlit_shading_model()
    : shading_model("Unlit", {SHADING_GBUFFER_ALBEDO, SHADING_GBUFFER_NORMAL})
{
    m_pipeline = {
        .compute_shader = render_device::instance().get_shader<unlit_cs>(),
    };
}
} // namespace violet