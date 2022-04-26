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
    virtual d3d12_resource* depth_stencil() = 0;

    virtual d3d12_render_target_proxy render_target_proxy() = 0;
    virtual d3d12_depth_stencil_proxy depth_stencil_proxy() = 0;

    DXGI_SAMPLE_DESC render_target_sample_desc() const noexcept { return m_sample_desc; }

protected:
    std::size_t back_buffer_index() const noexcept;

    d3d12_ptr<DXGISwapChain> m_swap_chain;
    std::size_t m_back_buffer_counter;

private:
    DXGI_SAMPLE_DESC m_sample_desc;
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
    virtual d3d12_resource* depth_stencil() override { return m_depth_stencil_buffer.get(); }

    virtual d3d12_render_target_proxy render_target_proxy() override
    {
        return render_target()->render_target();
    }
    virtual d3d12_depth_stencil_proxy depth_stencil_proxy() override
    {
        return depth_stencil()->depth_stencil();
    }

private:
    void resize_buffer(std::uint32_t width, std::uint32_t height);

    std::vector<std::unique_ptr<d3d12_render_target>> m_back_buffer;
    std::unique_ptr<d3d12_depth_stencil_buffer> m_depth_stencil_buffer;

    d3d12_ptr<DXGISwapChain> m_swap_chain;
};

class d3d12_swap_chain_multisample : public d3d12_swap_chain_base
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

    virtual d3d12_render_target_proxy render_target_proxy() override
    {
        return render_target()->render_target();
    }
    virtual d3d12_depth_stencil_proxy depth_stencil_proxy() override
    {
        return depth_stencil()->depth_stencil();
    }

private:
    void resize_buffer(std::uint32_t width, std::uint32_t height);

    std::size_t m_multiple_sampling;
    std::unique_ptr<d3d12_render_target_mutlisample> m_render_target;
    std::vector<d3d12_ptr<D3D12Resource>> m_back_buffer;
    std::unique_ptr<d3d12_depth_stencil_buffer> m_depth_stencil_buffer;
};

class d3d12_renderer : public renderer
{
public:
    d3d12_renderer(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling,
        D3D12GraphicsCommandList* command_list);

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command* allocate_command() override;
    virtual void execute(render_command* command) override;

    virtual resource* back_buffer() override;
    virtual resource* depth_stencil() override;
    virtual std::size_t adapter(adapter_info* infos, std::size_t size) const override;

    virtual void resize(std::uint32_t width, std::uint32_t height) override;

    void begin_frame(D3D12GraphicsCommandList* command_list);
    void end_frame(D3D12GraphicsCommandList* command_list);

    void present();

    DXGI_SAMPLE_DESC render_target_sample_desc() const noexcept
    {
        return m_swap_chain->render_target_sample_desc();
    }

private:
    std::unique_ptr<d3d12_swap_chain_base> m_swap_chain;

    std::vector<std::string> m_adapter_info;
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;
};
} // namespace ash::graphics::d3d12