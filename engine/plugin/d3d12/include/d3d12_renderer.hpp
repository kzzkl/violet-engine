#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"

namespace ash::graphics::d3d12
{
class d3d12_swap_chain
{
public:
    static constexpr DXGI_FORMAT RENDER_TARGET_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
    static constexpr DXGI_FORMAT DEPTH_STENCIL_FORMAT = DXGI_FORMAT_D24_UNORM_S8_UINT;

public:
    d3d12_swap_chain(HWND handle, std::uint32_t width, std::uint32_t height);
    virtual ~d3d12_swap_chain() = default;

    virtual void begin_frame(D3D12GraphicsCommandList* command_list);
    virtual void end_frame(D3D12GraphicsCommandList* command_list);

    void present();

    virtual d3d12_resource* back_buffer();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle();
    D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_handle();

    DXGI_SAMPLE_DESC render_target_sample_desc() const noexcept { return m_sample_desc; }

protected:
    d3d12_swap_chain(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling);

    UINT64 back_buffer_index() const noexcept;

    std::vector<std::unique_ptr<d3d12_back_buffer>> m_back_buffer;
    std::unique_ptr<d3d12_depth_stencil_buffer> m_depth_stencil_buffer;

    d3d12_ptr<DXGISwapChain> m_swap_chain;

    DXGI_SAMPLE_DESC m_sample_desc;
};

class d3d12_multisampling_swap_chain : public d3d12_swap_chain
{
public:
    d3d12_multisampling_swap_chain(
        HWND handle,
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling);

    virtual void begin_frame(D3D12GraphicsCommandList* command_list) override;
    virtual void end_frame(D3D12GraphicsCommandList* command_list) override;

    virtual d3d12_resource* back_buffer() override;

    virtual D3D12_CPU_DESCRIPTOR_HANDLE render_target_handle() override;

private:
    std::unique_ptr<d3d12_render_target> m_render_target;
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
    virtual std::size_t adapter(adapter_info* infos, std::size_t size) const override;

    void begin_frame(D3D12GraphicsCommandList* command_list);
    void end_frame(D3D12GraphicsCommandList* command_list);

    void present();

    DXGI_FORMAT render_target_format() const { return m_swap_chain->RENDER_TARGET_FORMAT; }

    DXGI_SAMPLE_DESC render_target_sample_desc() const noexcept
    {
        return m_swap_chain->render_target_sample_desc();
    }

private:
    std::unique_ptr<d3d12_swap_chain> m_swap_chain;

    std::vector<std::string> m_adapter_info;
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;
};
} // namespace ash::graphics::d3d12