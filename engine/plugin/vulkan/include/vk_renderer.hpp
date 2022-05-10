#pragma once

#include "vk_common.hpp"
#include "vk_pipeline.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_swap_chain
{
public:
    vk_swap_chain(VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height);
    ~vk_swap_chain();

    void resize(std::uint32_t width, std::uint32_t height);

    VkExtent2D extent() const noexcept { return m_extent; }

    VkFormat surface_format() const noexcept { return m_surface_format.format; }
    VkFormat depth_stencil_format() const noexcept { return m_depth_stencil_format; }

    vk_back_buffer* back_buffer();
    VkSwapchainKHR swap_chain() const noexcept { return m_swap_chain; }

private:
    void destroy();

    VkSurfaceFormatKHR choose_surface_format(VkSurfaceKHR surface);
    VkFormat choose_depth_stencil_format();
    VkPresentModeKHR choose_present_mode(VkSurfaceKHR surface);
    VkExtent2D choose_swap_extent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        std::uint32_t width,
        std::uint32_t height);

    VkSurfaceFormatKHR m_surface_format;
    VkFormat m_depth_stencil_format;
    VkPresentModeKHR m_present_mode;
    VkExtent2D m_extent;

    VkSwapchainKHR m_swap_chain;

    std::vector<vk_back_buffer> m_back_buffers;
};

class vk_renderer : public renderer_interface
{
public:
    vk_renderer(const renderer_desc& desc);

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command_interface* allocate_command() override;
    virtual void execute(render_command_interface* command) override;

    virtual resource_interface* back_buffer() override;

    virtual void resize(std::uint32_t width, std::uint32_t height) override;
};
} // namespace ash::graphics::vk