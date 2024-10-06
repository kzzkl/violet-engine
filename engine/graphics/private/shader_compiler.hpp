#pragma once

#include <span>
#include <string>
#include <vector>

#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#include <d3d12shader.h>
#include <dxcapi.h>
#include <wrl.h>
#endif

namespace violet
{
class shader_compiler
{
public:
    shader_compiler();

    std::vector<std::uint8_t> compile(std::span<char> source, std::span<const wchar_t*> arguments);

private:
    static void check(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw std::runtime_error("DXC compilation failed");
        }
    }

    Microsoft::WRL::ComPtr<IDxcCompiler3> m_compiler;
    Microsoft::WRL::ComPtr<IDxcUtils> m_utils;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_include_handler;
};
} // namespace violet