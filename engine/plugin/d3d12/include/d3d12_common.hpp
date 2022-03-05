#pragma once

#include "d3dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdexcept>
#include <string>
#include <wrl.h>

namespace ash::graphics::d3d12
{
using DXGIAdapter = IDXGIAdapter1;
using DXGIFactory = IDXGIFactory4;
using DXGISwapChain = IDXGISwapChain1;
using D3D12Device = ID3D12Device;
using D3D12CommandQueue = ID3D12CommandQueue;
using D3D12CommandAllocator = ID3D12CommandAllocator;
using D3D12Fence = ID3D12Fence;
using D3D12CommandList = ID3D12CommandList;
using D3D12GraphicsCommandList = ID3D12GraphicsCommandList;
using D3D12RootSignature = ID3D12RootSignature;
using D3D12Resource = ID3D12Resource;
using D3D12DescriptorHeap = ID3D12DescriptorHeap;
using D3DBlob = ID3DBlob;
using D3D12PipelineState = ID3D12PipelineState;

class hr_exception : public std::runtime_error
{
public:
    hr_exception(HRESULT hr) : std::runtime_error(hr_to_string(hr)), m_hr(hr) {}

    HRESULT error() const { return m_hr; }

private:
    std::string hr_to_string(HRESULT hr)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
        return std::string(s_str);
    }

    const HRESULT m_hr;
};

inline void throw_if_failed(HRESULT hr)
{
    if (FAILED(hr))
        throw hr_exception(hr);
}
} // namespace ash::graphics::d3d12