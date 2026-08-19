#include "graphics/shading_models/pbr_shading_model.hpp"
#include "graphics/render_device.hpp"
#include "graphics/resources/brdf_lut.hpp"

namespace violet
{
struct pbr_shading_model_constant
{
    std::uint32_t brdf_lut;
};

pbr_shading_model::pbr_shading_model()
    : shading_model("pbr_shading_model", sizeof(pbr_shading_model_constant))
{
    auto& device = render_device::instance();

    auto& constant = get_constant<pbr_shading_model_constant>();
    constant.brdf_lut = device.get_texture<brdf_lut>()->get_srv()->get_bindless();
}
} // namespace violet