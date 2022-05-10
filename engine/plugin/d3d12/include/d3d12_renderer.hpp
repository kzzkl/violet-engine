#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"

namespace ash::graphics::d3d12
{
class d3d12_swap_chain_base
{
public:
    d3d12_swap_chain_base(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1);
    virtual ~d3d12_swap_chain_base() = default;

    virtual void begin_frame(D3D12GraphicsCommandList* command_list) = 0;
    virtual void end_frame(D3D12GraphicsCommandList* command_list) = 0;

    virtual void resize(std::uint32_t width, std::uint32_t height);

    void present();

    virtual d3d12_resource* render_target() = 0;

protected:
    std::size_t back_buffer_index() const noexcept;

    d3d12_ptr<DXGISwapChain> m_swap_chain;
    std::size_t m_back_buffer_counter;
};

class d3d12_swap_chain : public d3d12_swap_chain_base
{
public:
    d3d12_swap_chain(HWND handle, std::uint32_t width, std::uint32_t height);
    virtual ~d3d12_swap_chain() = default;

    virtual void begin_frame(D3D12GraphicsCommandList* command_list) override;
    virtual void end_frame(D3D12GraphicsCommandList* command_list) override;
    virtual void resize(std::uint32_t width, std::uint32_t height) override;

    virtual d3d12_resource* render_target() override
    {
        return m_back_buffer[back_buffer_index()].get();
    }

private:
    void resize_buffer(std::uint32_t width, std::uint32_t height);
    std::vector<std::unique_ptr<d3d12_render_target>> m_back_buffer;
};

/*class d3d12_swap_chain_multisample : public d3d12_swap_chain_base
{
public:
    d3d12_swap_chain_multisample(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling);

    virtual void begin_frame(D3D12GraphicsCommandList* command_list) override;
    virtual void end_frame(D3D12GraphicsCommandList* command_list) override;

    virtual d3d12_resource* render_target() override { return m_render_target.get(); }
    virtual d3d12_resource* depth_stencil() override { return m_depth_stencil_buffer.get(); }
    virtual void resize(std::uint32_t width, std::uint32_t height) override;

private:
    void resize_buffer(std::uint32_t width, std::uint32_t height);

    std::size_t m_multiple_sampling;
    std::unique_ptr<d3d12_render_target> m_render_target;
    std::vector<d3d12_ptr<D3D12Resource>> m_back_buffer;
    std::unique_ptr<d3d12_depth_stencil_buffer> m_depth_stencil_buffer;
};*/

class d3d12_renderer : public renderer_interface
{
public:
    d3d12_renderer(const renderer_desc& desc);

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command_interface* allocate_command() override;
    virtual void execute(render_command_interface* command) override;

    virtual resource_interface* back_buffer() override;

    virtual void resize(std::uint32_t width, std::uint32_t height) override;

    // void begin_frame(D3D12GraphicsCommandList* command_list);
    // void end_frame(D3D12GraphicsCommandList* command_list);

private:
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;
};
} // namespace ash::graphics::d3d12