#pragma once

#include "vk_common.hpp"
#include "vk_pipeline.hpp"

namespace ash::graphics::vk
{
class vk_swap_chain
{
public:
    vk_swap_chain(VkSurfaceKHR surface, std::uint32_t width, std::uint32_t height);
    ~vk_swap_chain();

    VkExtent2D extent() const noexcept { return m_extent; }

    VkFormat format() const noexcept { return m_surface_format.format; }
    const std::vector<VkImageView>& image_views() { return m_image_views; }

    VkSwapchainKHR swap_chain() const noexcept { return m_swap_chain; }

private:
    VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choose_present_mode(const std::vector<VkPresentModeKHR>& modes);
    VkExtent2D choose_swap_extent(
        const VkSurfaceCapabilitiesKHR& capabilities,
        std::uint32_t width,
        std::uint32_t height);

    VkSurfaceFormatKHR m_surface_format;
    VkPresentModeKHR m_present_mode;
    VkExtent2D m_extent;

    VkSwapchainKHR m_swap_chain;

    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_image_views;
};

class vk_renderer : public renderer
{
public:
    vk_renderer();

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command* allocate_command() override { return nullptr; }
    virtual void execute(render_command* command) override {}

    virtual resource* back_buffer() override { return nullptr; }
    virtual resource* depth_stencil() override { return nullptr; }
    virtual std::size_t adapter(adapter_info* infos, std::size_t size) const override { return 0; }

    virtual void resize(std::uint32_t width, std::uint32_t height) override {}
};
} // namespace ash::graphics::vk