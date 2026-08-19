#include "graphics/shading_model.hpp"
#include "common/utility.hpp"
#include <format>

namespace violet
{
shading_model::shading_model(
    std::string_view name,
    std::size_t constant_size,
    std::string_view shader_path)
    : m_name(name),
      m_constant(constant_size)
{
    if (shader_path.empty())
    {
        m_defines.push_back(
            std::format(
                L"-DSHADING_MODEL_SHADER=\"shading_models/{}.hlsli\"",
                string_to_wstring(m_name)));
    }
    else
    {
        m_defines.push_back(
            std::format(L"-DSHADING_MODEL_SHADER=\"{}\"", string_to_wstring(shader_path)));
    }

    m_defines.push_back(std::format(L"-DSHADING_MODEL_NAME={}", string_to_wstring(m_name)));

    if (constant_size != 0)
    {
        m_defines.push_back(std::format(L"-DSHADING_MODEL_HAS_CONSTANT"));
    }
}
} // namespace violet