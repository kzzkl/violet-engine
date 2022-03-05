#pragma once

#include "d3d12_common.hpp"
#include "graphics_interface.hpp"
#include "d3d12_resource.hpp"

namespace ash::graphics::d3d12
{
class d3d12_renderer : public ash::graphics::external::renderer
{
public:
    d3d12_renderer();

    bool initialize(HWND handle, UINT width, UINT height);

    virtual void begin_frame() override;
    virtual void end_frame() override;

private:
    UINT64 get_index() const { return m_frame_counter % 2; }

    static const DXGI_FORMAT m_format{DXGI_FORMAT_R8G8B8A8_UNORM};

    Microsoft::WRL::ComPtr<DXGISwapChain> m_swap_chain;

    std::vector<d3d12_resource> m_back_buffer;
    d3d12_resource m_depth_stencil_buffer;

    // D3D12_CPU_DESCRIPTOR_HANDLE m_depth_stencil_buffer_view;
    // Microsoft::WRL::ComPtr<D3D12Resource> m_depth_stencil_buffer;

    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissor_rect;

    UINT64 m_frame_counter;
};
} // namespace ash::graphics::d3d12