#pragma once

#include "graphics.hpp"
#include "render_pipeline.hpp"

namespace ash::sample::mmd
{
class mmd_pass : public graphics::render_pass
{
public:
    mmd_pass();
    virtual void render(const graphics::camera& camera, graphics::render_command_interface* command)
        override;

    void resize(std::uint32_t width, std::uint32_t height);

private:
    void initialize_interface();

    std::unique_ptr<graphics::render_pass_interface> m_interface;

    std::uint32_t m_width;
    std::uint32_t m_height;
    std::size_t m_counter;
};
} // namespace ash::sample::mmd