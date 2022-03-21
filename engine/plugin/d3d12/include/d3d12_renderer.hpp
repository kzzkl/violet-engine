#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"

namespace ash::graphics::d3d12
{
class d3d12_renderer : public renderer
{
public:
    d3d12_renderer(HWND handle, UINT width, UINT height, D3D12GraphicsCommandList* command_list);

    virtual void begin_frame() override;
    virtual void end_frame() override;

    virtual render_command* allocate_command() override;
    virtual void execute(render_command* command) override;

    virtual resource* back_buffer() override;
    virtual std::size_t adapter(adapter_info* infos, std::size_t size) const override;

    void begin_frame(D3D12GraphicsCommandList* command_list);
    void end_frame(D3D12GraphicsCommandList* command_list);

    void present();

private:
    UINT64 back_buffer_index() const noexcept;

    static const DXGI_FORMAT m_format = DXGI_FORMAT_R8G8B8A8_UNORM;

    d3d12_ptr<DXGISwapChain> m_swap_chain;

    std::vector<std::unique_ptr<d3d12_back_buffer>> m_back_buffer;
    std::unique_ptr<d3d12_depth_stencil_buffer> m_depth_stencil_buffer;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;

    std::vector<std::string> m_adapter_info;
};
} // namespace ash::graphics::d3d12