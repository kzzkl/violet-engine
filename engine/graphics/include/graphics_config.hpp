#pragma once

#include "dictionary.hpp"
#include "graphics_interface.hpp"
#include <map>
#include <string_view>
#include <vector>

namespace ash::graphics
{
class graphics_config
{
public:
    graphics_config();

    void load(const dictionary& config);

    inline std::size_t render_concurrency() const noexcept { return m_render_concurrency; }
    inline std::size_t frame_resource() const noexcept { return m_frame_resource; }
    inline std::size_t samples() const noexcept { return m_samples; }

    inline std::string_view plugin() const noexcept { return m_plugin; }

private:
    std::size_t m_render_concurrency;
    std::size_t m_frame_resource;
    std::size_t m_samples;

    std::string m_plugin;
};
} // namespace ash::graphics