#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"

namespace ash::graphics::d3d12
{
class d3d12_swap_chain
{
public:
    d3d12_swap_chain(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1);
    ~d3d12_swap_chain();

    void resize(std::uint32_t width, std::uint32_t height);
    void present();

    d3d12_resource* render_target() { return m_back_buffers[back_buffer_index()].get(); }

protected:
    void resize_buffer(std::uint32_t width, std::uint32_t height);
    void reset_back_buffers();
    std::size_t back_buffer_index() const noexcept;

    d3d12_ptr<DXGISwapChain> m_swap_chain;

    std::size_t m_back_buffer_counter;
    std::vector<std::unique_ptr<d3d12_back_buffer>> m_back_buffers;
};

class d3d12_renderer : public renderer_interface
{
public:
    d3d12_renderer(const renderer_desc& desc);
    virtual ~d3d12_renderer();

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command_interface* allocate_command() override;
    virtual void execute(render_command_interface* command) override;

    virtual resource_interface* back_buffer() override;

    virtual void resize(std::uint32_t width, std::uint32_t height) override;
};
} // namespace ash::graphics::d3d12